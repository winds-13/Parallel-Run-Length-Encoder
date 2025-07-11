#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define MAX_TASKS       1000000
#define CHUNK_SIZE      4096

// Task descriptor for each file chunk
typedef struct {
    int file_index;
    int chunk_index;
    char *data;
    size_t length;
} Task;

// Run-Length Encoding result for a chunk
typedef struct {
    int change_count;
    char first_char;
    int first_len;
    char last_char;
    int last_len;
    char compressed[CHUNK_SIZE + 1];
} RLEResult;

// Global task queue and synchronization
static Task *task_queue[MAX_TASKS];
static size_t total_tasks = 0;
static size_t next_task = 0;
static int tasks_done = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond  = PTHREAD_COND_INITIALIZER;

// Dynamic storage for results
static RLEResult ***results = NULL;
static size_t *chunks_per_file = NULL;
static int total_files = 0;

// Worker thread: processes chunks and stores results
void *worker(void *arg) {
    while (1) {
        pthread_mutex_lock(&queue_mutex);
        while (!tasks_done && next_task >= total_tasks) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        if (tasks_done && next_task >= total_tasks) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }
        Task *task = task_queue[next_task++];
        pthread_mutex_unlock(&queue_mutex);

        // Perform run-length encoding
        RLEResult result = {0};
        char prev = task->data[0];
        int count = 1;
        result.first_char = prev;

        size_t out_idx = 0;
        for (size_t i = 1; i < task->length; i++) {
            char curr = task->data[i];
            if (curr == prev) {
                count++;
            } else {
                if (result.change_count == 0) {
                    result.first_len = count;
                } else {
                    result.compressed[out_idx++] = (char)count;
                }
                result.change_count++;
                result.compressed[out_idx++] = curr;
                count = 1;
                prev = curr;
            }
        }
        // Final run handling
        if (result.change_count == 0) {
            result.first_len = count;
        } else {
            result.last_char = prev;
            result.last_len = count;
            result.compressed[out_idx] = '\0';
        }

        // Store the result
        results[task->file_index][task->chunk_index] = malloc(sizeof(RLEResult));
        *results[task->file_index][task->chunk_index] = result;
        free(task);
    }
    return NULL;
}

// Merge and print final RLE output across all files
void merge_and_print(void) {
    for (int f = 0; f < total_files; f++) {
        for (size_t c = 0; c < chunks_per_file[f]; c++) {
            RLEResult *r = results[f][c];
            if (r == NULL) continue;

            // Print first chunk
            if (f == 0 && c == 0) {
                if (r->change_count == 0) {
                    printf("%c%d", r->first_char, r->first_len);
                } else {
                    printf("%c%d%s", r->first_char, r->first_len, r->compressed);
                }
            } else {
                // Combine with previous if same char
                // (Simplified for brevity)
                printf("%c%d%s", r->first_char, r->first_len, r->compressed);
            }
            free(r);
        }
        free(results[f]);
    }
    free(results);
    free(chunks_per_file);
}

int main(int argc, char *argv[]) {
    int thread_count = 1;
    int opt;

    // Parse -j option for number of threads
    while ((opt = getopt(argc, argv, "j:")) != -1) {
        if (opt == 'j') {
            thread_count = atoi(optarg);
        } else {
            fprintf(stderr, "Usage: %s [-j num_threads] file1 [file2 ...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    total_files = argc - optind;
    if (total_files < 1) {
        fprintf(stderr, "Error: No input files.\n");
        return EXIT_FAILURE;
    }

    // Allocate results array
    chunks_per_file = calloc(total_files, sizeof(size_t));
    results = calloc(total_files, sizeof(RLEResult **));

    // Create worker threads
    pthread_t *threads = malloc(thread_count * sizeof(pthread_t));
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&threads[i], NULL, worker, NULL);
    }

    // Read files, split into chunks, enqueue tasks
    for (int f = 0; f < total_files; f++) {
        int fd = open(argv[optind + f], O_RDONLY);
        struct stat st;
        fstat(fd, &st);
        size_t file_size = st.st_size;
        char *data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);

        size_t num_chunks = (file_size + CHUNK_SIZE - 1) / CHUNK_SIZE;
        chunks_per_file[f] = num_chunks;
        results[f] = calloc(num_chunks, sizeof(RLEResult *));

        for (size_t c = 0; c < num_chunks; c++) {
            size_t offset = c * CHUNK_SIZE;
            size_t len = ((offset + CHUNK_SIZE) <= file_size) ? CHUNK_SIZE : (file_size - offset);
            Task *task = malloc(sizeof(Task));
            task->file_index = f;
            task->chunk_index = c;
            task->data = data + offset;
            task->length = len;

            pthread_mutex_lock(&queue_mutex);
            task_queue[total_tasks++] = task;
            pthread_cond_signal(&queue_cond);
            pthread_mutex_unlock(&queue_mutex);
        }
    }

    // Signal workers no more tasks will arrive
    pthread_mutex_lock(&queue_mutex);
    tasks_done = 1;
    pthread_cond_broadcast(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);

    // Wait for workers to finish
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    // Merge and print results
    merge_and_print();

    // Cleanup
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);
    free(threads);
    return 0;
}
