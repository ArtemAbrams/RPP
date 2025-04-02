#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

typedef unsigned long long ull;
typedef long long ll;

#define WIDTH 3000
#define HEIGHT 3000

const ll p_const = 3000000007LL;
const ll q_const = 3000000011LL;
const ll n_const = p_const * q_const;
const ll e_const = 900000000000000LL;

ull modexp(ull base, ll exp, ull mod) {
    ull result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1)
            result = (result * base) % mod;
        exp >>= 1;
        base = (base * base) % mod;
    }
    return result;
}

int main() {
    int i, j;

    ull global_min = n_const;
    ull global_max = 0;

    ull *data = malloc(WIDTH * HEIGHT * sizeof(ull));
    if (!data) {
        fprintf(stderr, "Помилка виділення пам'яті!\n");
        return 1;
    }

    double t0 = omp_get_wtime();

    long long max_threads = omp_get_max_threads();
    long long *rows_processed = calloc(max_threads, sizeof(long long));
    if (!rows_processed) {
        fprintf(stderr, "Помилка виділення rows_processed!\n");
        free(data);
        return 1;
    }

    #pragma omp parallel for private(j) \
        reduction(min:global_min) \
        reduction(max:global_max) \
        schedule(dynamic)
    for (i = 0; i < HEIGHT; i++) {
        int tid = omp_get_thread_num();
        rows_processed[tid]++;
        for (j = 0; j < WIDTH; j++) {
            ll message = (i + j) * WIDTH;
            ull ciphertext = modexp(message, e_const, n_const);
            data[i * WIDTH + j] = ciphertext;

            if (ciphertext < global_min) global_min = ciphertext;
            if (ciphertext > global_max) global_max = ciphertext;
        }
    }

    double t1 = omp_get_wtime();

    printf("Згенеровано зображення %dx%d за %f секунд. min = %llu, max = %llu\n",
           WIDTH, HEIGHT, t1 - t0, global_min, global_max);
    for (i = 0; i < max_threads; i++) {
        printf("Thread %d обробив %lld рядків\n", i, rows_processed[i]);
    }

    printf("Верхній лівий: %llu\n", data[0]);
    printf("Верхній правий: %llu\n", data[WIDTH - 1]);
    printf("Нижній лівий: %llu\n", data[(HEIGHT - 1) * WIDTH]);
    printf("Нижній правий: %llu\n", data[HEIGHT * WIDTH - 1]);
    printf("Центр: %llu\n", data[(HEIGHT / 2) * WIDTH + (WIDTH / 2)]);

    free(rows_processed);
    free(data);

    return 0;
}