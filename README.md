# Parallel-Run-Length-Encoder

Parallel Run-Length Encoder (Linux, C, Multi-threading, IPC, Thread Pool, pthreads)                                                                 2023.02 - 2023.03

•	Developed a high-performance multi-threaded run-length encoding program in C by implementing a robust thread pool using pthreads; achieved up to 75% reduction in running time compared to the single-threaded version on large data sets over 1GB

•	Leveraged the concept of task queues, designed custom task data structures to ensure even task workload and efficient future merging process; Employed dynamic scheduling allowing simultaneous task encoding and merging, reduced processing latency 

•	Utilized mutexes and condition variables to prevent race conditions, busy waiting, and deadlocks on shared data structures; Used fined-grained and read-write locks to minimize the time spent holding mutex locks, reduced contention among threads
