#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#define MAX_COUNT 999999

typedef struct Task {
    int index;
    int index2;
    int start;
    int end;
    char file[256];
    char* string;
    int forward_len;
} Task;

typedef struct Sentence {
    int change_time;
    char last_char;
    int last_char_len;
    char fst_char;
    int fst_char_len;
    char converted[4097];
    int isEmpty;
} Sentence;
pthread_mutex_t mutexTaskArr;
pthread_mutex_t mutexResArr;
pthread_cond_t condTaskArr;
int taskCount=0;
int completedTasks = 0;
int file_num;
int added_task_num=0;
int completed = 0;
int empty[256];
    int empty_num=0;
int task_index=0;
Sentence* res_arr[999][99999];

Task* task_arr[MAX_COUNT];
int tempCount=0;
int chunk_num_arr[999];

void* startThread(void* args) {
    while(1){ 
        pthread_mutex_lock(&mutexTaskArr);
        // printf("completed = %d\n", completed);
        // printf("Taskcount = %d\n", taskCount);
        // printf("TaskIndex = %d\n\n", task_index);

        while (taskCount <= task_index) {
            if (completed == 1 && taskCount >= task_index) {
                pthread_mutex_unlock(&mutexTaskArr);
                return NULL;
            }
            pthread_cond_wait(&condTaskArr, &mutexTaskArr);
        }

        char task_file[256];
        int index,index2, start_ind, end_ind, forward_len;
        char* addr;
        int count=1; 
        index=task_arr[task_index]->index;
        index2=task_arr[task_index]->index2;
        forward_len=task_arr[task_index]->forward_len;
        addr=task_arr[task_index]->string;
        task_index++;
        pthread_mutex_unlock(&mutexTaskArr);

        // printf("index = %d\n", index);
        // printf("index2 = %d\n", index2);
        // printf("forward = %d\n", forward_len);
        // printf("end = %d\n\n\n", end_ind);

        Sentence *s = (Sentence *)malloc(sizeof(Sentence));
        int char_sep = 1;
        char curr_char, last_char;
        int middle_ind=0;
        // // ahhhhhhhhhhh!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        for (int i=0; i<forward_len; i++){
                curr_char = *(addr+i);

                if (i==0) { 
                    last_char = curr_char;
                    s->fst_char=curr_char;
                }else{
                    if (curr_char != last_char) {
                        if(char_sep==1){
                            s->fst_char_len=count;
                        }else{
                            s->converted[middle_ind++]=(char)count;
                        }

                        if((i+1)!=forward_len){
                            s->converted[middle_ind++]=curr_char;
                        }else{
                            s->last_char=curr_char;
                        }
                        count=1;
                        char_sep++;
                        
                    }else{
                        count++;
                        if((i+1)==forward_len && char_sep!=1){
                            s->converted[middle_ind-1]='\0';
                            s->last_char=curr_char;
                            s->last_char_len=count;
                        }
                    }
                }
                last_char = curr_char;
            
        }
        if(char_sep==1){
            s->fst_char_len=count;
        }else{
            s->last_char_len=count;
        }
        s->change_time=char_sep;
        s->isEmpty = 0;

        pthread_mutex_lock(&mutexResArr);
        res_arr[index][index2]=s;
        pthread_mutex_unlock(&mutexResArr);


        pthread_mutex_lock(&mutexTaskArr);
        if (completed == 1 && taskCount >= task_index) {
            pthread_cond_broadcast(&condTaskArr);
        }
        pthread_mutex_unlock(&mutexTaskArr);
    }
}
int cur_ind=0;
int cur_ind2=0;
int lastChar;
int lastNum;
int lastChange;
void merge(){
    while(1){
        // printf("ind1 = %d\n", cur_ind);
        // printf("ind2 = %d\n\n", cur_ind2);
        // printf("chunkarr = %d\n\n", chunk_num_arr[cur_ind]);
        if(cur_ind2>=chunk_num_arr[cur_ind]){
            // free(task_arr[cur_ind]);
            // printf("\ncna1 = %d\n", cur_ind);
            // printf("\ncna2 = %d\n", chunk_num_arr[cur_ind]);
            for (int i=0; i<chunk_num_arr[cur_ind];i++){
                // free(res_arr[cur_ind][i]);
            }
            cur_ind++;
            cur_ind2=0;
        }
        if(cur_ind==file_num){
            printf("%c%c", lastChar, (char)lastNum);
            break;
        }
        pthread_mutex_lock(&mutexResArr);
        if(res_arr[cur_ind][cur_ind2]==NULL){
            pthread_mutex_unlock(&mutexResArr);
            continue;
        }
        pthread_mutex_unlock(&mutexResArr);
        pthread_mutex_lock(&mutexResArr);
        Sentence *cur = res_arr[cur_ind][cur_ind2];
        pthread_mutex_unlock(&mutexResArr);
        int change_time = cur->change_time;
        char last_char = cur->last_char;
        int last_char_len = cur->last_char_len;
        char fst_char = cur->fst_char;
        int fst_char_len = cur->fst_char_len;
        char converted[4097];
        strcpy(converted, cur->converted);
        int isEmpty = cur->isEmpty;

        if (isEmpty==1){
            cur_ind2++;
            continue;
        }
        if(cur_ind==0 && cur_ind2==0){
            if (change_time==1){
                lastChar=fst_char;
                lastNum=fst_char_len;
                lastChange=change_time;
            }else if(change_time==2){
                printf("%c%c", fst_char, (char)fst_char_len);
                lastChar=last_char;
                lastNum=last_char_len;
                lastChange=change_time;
            }else{
                printf("%c%c", fst_char, (char)fst_char_len);
                printf("%s", converted);
                lastChar=last_char;
                lastNum=last_char_len;
                lastChange=change_time;
            }
            
        }else{
            if (change_time==1){
                if(fst_char==lastChar){
                    lastNum+=fst_char_len;
                }else{
                    printf("%c%c", lastChar, (char)(lastNum));
                    lastNum=fst_char_len;
                    lastChar=fst_char;
                    lastChange=1;
                }
            }else if(change_time==2){
                if(fst_char==lastChar){
                    printf("%c%c", fst_char, (char)(fst_char_len+lastNum));
                }else{
                    printf("%c%c", last_char, (char)(last_char_len));
                    printf("%c%c", fst_char, (char)(fst_char_len));
                    lastNum=last_char_len;
                    lastChar=last_char;
                    lastChange=2;
                }
            }else{
                if(fst_char==lastChar){
                    printf("%c%c", fst_char, (char)(fst_char_len+lastNum));
                }else{
                    printf("%c%c", lastChar, (char)(lastNum));
                    printf("%c%c", fst_char, (char)(fst_char_len));
                }
                printf("%s", converted);
                lastChar=last_char;
                lastNum=last_char_len;
                lastChange=change_time;
            }
        
        }
        cur_ind2++;
        
    }
}

int main(int argc, char *argv[]) {
    int option;
    int i;
    for (int i=0;i<argc; i++){
        if(strcmp(argv[i], ">")==0){
            argc=i+1;
            break;
        }
    }
    
    int index_start=1;
    int thread_num=1;
    while ((option = getopt(argc, argv, "j:")) != -1) {
        switch (option) {
            case 'j':
                index_start=3;
                thread_num = atoi(argv[2]);
                break;
            default:
                exit(0);
        }
    }
  

    file_num = argc-index_start;
    pthread_t th[thread_num];
    pthread_mutex_init(&mutexTaskArr, NULL);
    pthread_mutex_init(&mutexResArr, NULL);
    pthread_cond_init(&condTaskArr, NULL);
    
    int temp_argc = argc;
    int j;
    for (j = 0; j < thread_num; j++) {
        if (pthread_create(&th[j], NULL, &startThread, NULL) != 0) {
            perror("Failed to create the thread");
        }
    }
    
    for (int j=index_start;j<argc; j++){
        int fd = open(argv[j], O_RDONLY);
        // int char_num = lseek(fd, 0, SEEK_END);
        struct stat sb;
        fstat(fd, &sb);
        int char_num = sb.st_size;
        int temp_char_num=char_num;
        void *addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if(char_num==0){
            continue;
        }
        // printf("char_num = %d\n", char_num);
        // printf("char_num2 = %d\n", sb.st_size);
        
        int chunk_num;
        if(sb.st_size%4096==0){
            chunk_num=sb.st_size/4096;
        }else{
            chunk_num=sb.st_size/4096+1;
        }
        chunk_num_arr[j-index_start]=chunk_num;
        int chunk_ind = 0;
        int curStart=0;
        int i=0;
        int next=0;
        while(temp_char_num>=4096){
            Task* t= (Task *) malloc(sizeof(Task));
            t->index=j-index_start;
            t->index2=i++;
            t->string=addr+next;
            t->forward_len=4096;
            next+=4096;
            temp_char_num-=4096;
            pthread_mutex_lock(&mutexTaskArr);
            task_arr[taskCount++]=t;
            pthread_cond_signal(&condTaskArr);
            pthread_mutex_unlock(&mutexTaskArr);
        }
        if(temp_char_num!=0){
            Task* t= (Task *) malloc(sizeof(Task));
            t->index=j-index_start;
            t->index2=i++;
            t->string=addr+next;
            t->forward_len=temp_char_num;
            pthread_mutex_lock(&mutexTaskArr);
            task_arr[taskCount++]=t;
            pthread_cond_signal(&condTaskArr);
            pthread_mutex_unlock(&mutexTaskArr);
        }

      
    }
    
            

    pthread_mutex_lock(&mutexTaskArr);
    completed=1;
    // pthread_cond_broadcast(&condTaskArr);
    pthread_mutex_unlock(&mutexTaskArr);

    merge();
   
    
    for (j = 0; j < thread_num; j++) {
        if (pthread_join(th[j], NULL) != 0) {
            perror("Failed to join the thread");
        }
    }

    // for (int i=0; i<taskCount; i++){
    //     free(task_arr[task_index]);
    // }
    // for (int i=0; i<file_num; i++){
    //     for (int j=0; j<chunk_num_arr[i];j++){
    //         free(res_arr[i][j]);
    //     }
    // }

    pthread_mutex_destroy(&mutexTaskArr);
    pthread_mutex_destroy(&mutexResArr);
    pthread_cond_destroy(&condTaskArr);
    return 0;
}