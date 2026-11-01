#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

typedef struct {
    int strategy;
    int start_idx, end_idx;
    int start_k, end_k;
    int K, N;
    int **arrays;
    int *result;
    pthread_mutex_t *mutex;
} ThreadArgs;

void free_memory(int **arrays, int K, int *result) {
    if (arrays != NULL) {
        for (int i = 0; i < K; i++) {
            if (arrays[i] != NULL) free(arrays[i]);
        }
        free(arrays);
    }
    if (result != NULL) free(result);
}

void* thread_func(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    
    if (args->strategy == 1) {
        for (int i = args->start_idx; i < args->end_idx; i++) {
            args->result[i] = 0;
        }
        for (int j = 0; j < args->K; j++) {
            for (int i = args->start_idx; i < args->end_idx; i++) {
                args->result[i] += args->arrays[j][i];
            }
        }
    } else {
        int *local_result = (int*)calloc(args->N, sizeof(int));
        for (int k = args->start_k; k < args->end_k; k++) {
            for (int i = 0; i < args->N; i++) {
                local_result[i] += args->arrays[k][i];
            }
        }
        
        pthread_mutex_lock(args->mutex);
        for (int i = 0; i < args->N; i++) {
            args->result[i] += local_result[i];
        }
        pthread_mutex_unlock(args->mutex);
        
        free(local_result);
    }
    return NULL;
}

void sequential_add(int K, int N, int **arrays, int *result) {
    for (int j = 0; j < K; j++) {
        for (int i = 0; i < N; i++) {
            result[i] += arrays[j][i];
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("%s <K> <N> <max_threads>\n", argv[0]);
        return 1;
    }

    int K = atoi(argv[1]);
    int N = atoi(argv[2]);
    int max_threads = atoi(argv[3]);

    if (K <= 0 || N <= 0 || max_threads <= 0) {
        printf("Ошибка: все аргументы должны быть положительными числами\n");
        return 1;
    }

    unsigned long long total_elements = (unsigned long long)K * N;
    unsigned long long data_bytes = total_elements * sizeof(int);
    unsigned long long ptr_bytes = (unsigned long long)K * sizeof(int*);
    unsigned long long result_bytes = (unsigned long long)N * sizeof(int);
    unsigned long long total_needed = data_bytes + ptr_bytes + result_bytes;

    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    unsigned long long total_ram = (unsigned long long)pages * page_size;

    if (total_needed > total_ram) {
        printf("Ошибка: Требуется %llu байт, а в системе доступно только %llu байт\n", total_needed, total_ram);
        return 1;
    }

    printf("Параметры: K=%d, N=%d, max_threads=%d\n", K, N, max_threads);

    int **arrays = (int**)malloc(K * sizeof(int*));
    if (arrays == NULL) {
        printf("Ошибка выделения памяти под массив указателей\n");
        return 1;
    }

    for (int i = 0; i < K; i++) {
        arrays[i] = (int*)malloc(N * sizeof(int));
        if (arrays[i] == NULL) {
            printf("Ошибка: не хватило памяти под массив %d\n", i);
            free_memory(arrays, i, NULL);
            return 1;
        }
        for (int j = 0; j < N; j++) {
            arrays[i][j] = 1;
        }
    }

    int *result = (int*)calloc(N, sizeof(int));
    if (result == NULL) {
        printf("Ошибка выделения памяти под результат\n");
        free_memory(arrays, K, NULL);
        return 1;
    }

    struct timeval start_seq, end_seq;
    gettimeofday(&start_seq, NULL);

    sequential_add(K, N, arrays, result);

    gettimeofday(&end_seq, NULL);
    double time_seq = (end_seq.tv_sec - start_seq.tv_sec) + (end_seq.tv_usec - start_seq.tv_usec) / 1000000.0;
    printf("Время последовательной версии: %f сек\n", time_seq);
    
    int *result_par = (int*)calloc(N, sizeof(int));
    if (result_par == NULL) {
        printf("Ошибка выделения памяти под параллельный результат\n");
        free_memory(arrays, K, result);
        return 1;
    }

    int strategy = (K > N) ? 2 : 1;
    int num_threads = max_threads;
    
    if (strategy == 1) {
        if (num_threads > N) num_threads = N;
    } else {
        if (num_threads > K) num_threads = K;
    }

    pthread_t *threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    ThreadArgs *args = (ThreadArgs*)malloc(num_threads * sizeof(ThreadArgs));
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    struct timeval start_par, end_par;
    gettimeofday(&start_par, NULL);

    if (strategy == 1) {
        int chunk_size = N / num_threads;
        int remainder = N % num_threads;
        int current_idx = 0;

        for (int i = 0; i < num_threads; i++) {
            args[i].strategy = 1;
            args[i].start_idx = current_idx;
            args[i].end_idx = current_idx + chunk_size + (i < remainder ? 1 : 0);
            args[i].K = K;
            args[i].N = N;
            args[i].arrays = arrays;
            args[i].result = result_par;
            args[i].mutex = &mutex;
            
            current_idx = args[i].end_idx;
            pthread_create(&threads[i], NULL, thread_func, &args[i]);
        }
    } else {
        int chunk_size = K / num_threads;
        int remainder = K % num_threads;
        int current_k = 0;

        for (int i = 0; i < num_threads; i++) {
            args[i].strategy = 2;
            args[i].start_k = current_k;
            args[i].end_k = current_k + chunk_size + (i < remainder ? 1 : 0);
            args[i].K = K;
            args[i].N = N;
            args[i].arrays = arrays;
            args[i].result = result_par;
            args[i].mutex = &mutex;
            
            current_k = args[i].end_k;
            pthread_create(&threads[i], NULL, thread_func, &args[i]);
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_mutex_destroy(&mutex);

    gettimeofday(&end_par, NULL);
    double time_par = (end_par.tv_sec - start_par.tv_sec) + (end_par.tv_usec - start_par.tv_usec) / 1000000.0;

    printf("Время параллельной версии: %f сек\n", time_par);
    
    double speedup = time_seq / time_par;
    double efficiency = speedup / num_threads;

    printf("S: %f\n", speedup);
    printf("X: %f\n", efficiency);

    free(result_par);
    free(threads);
    free(args);
    free_memory(arrays, K, result);

    return 0;
}