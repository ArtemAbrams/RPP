#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <limits.h>
#include <tgmath.h>

#define WIDTH  3000
#define HEIGHT 3000

typedef unsigned long long ull;
typedef long long ll;

// RSA константи
const ll p_const = 3000000007LL;
const ll q_const = 3000000011LL;
const ll n_const = p_const * q_const;
const ll e_const = 900000000000000LL;

// Функція швидкого піднесення до степеня за модулем
ull modexp(ull base, ull exp, ull mod) {
    ull result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1) result = (result * base) % mod;
        exp >>= 1;
        base = (base * base) % mod;
    }
    return result;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int width, height;
    ull n;
    ll e;
    if (rank == 0) {
        width = WIDTH;
        height = HEIGHT;
        n = n_const;
        e = e_const;
    }

    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&n, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&e, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    int rows_per_proc = HEIGHT / size;
    int remainder = HEIGHT % size;
    int start_row = (rank < remainder)
                        ? rank * (rows_per_proc + 1)
                        : rank * rows_per_proc + remainder;
    int local_rows = (rank < remainder)
                         ? rows_per_proc + 1
                         : rows_per_proc;
    int end_row = start_row + local_rows - 1;
    printf("Process %d/%d: rows %d to %d (total %d)\n",
           rank, size, start_row, end_row, local_rows);

    ull local_min = ULLONG_MAX;
    ull local_max = 0;
    double start_time = MPI_Wtime();

    ull *local_data = malloc(local_rows * WIDTH * sizeof(ull));
    for (int i = 0; i < local_rows; i++) {
        int global_row = start_row + i;
        if (global_row == 500) {
            printf("Процес %d: обробка глобального рядка %d\n", rank, global_row);
        }
        for (int j = 0; j < WIDTH; j++) {
            ll message = (global_row + j) * WIDTH;
            ull ciphertext = modexp((ull) message, e, n);

            local_data[i * WIDTH + j] = ciphertext;
            if (ciphertext < local_min) local_min = ciphertext;
            if (ciphertext > local_max) local_max = ciphertext;
        }
    }

    ull global_min, global_max;
    double global_time;
    MPI_Reduce(&local_min, &global_min, 1, MPI_UNSIGNED_LONG_LONG, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_max, &global_max, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, 0, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();
    double local_time = end_time - start_time;
    MPI_Reduce(&local_time, &global_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    int *recvcounts = NULL, *displs = NULL;
    ull *global_data = NULL;
    if (rank == 0) {
        global_data = malloc(HEIGHT * WIDTH * sizeof(ull));
        recvcounts = malloc(size * sizeof(int));
        displs = malloc(size * sizeof(int));
        for (int p = 0; p < size; p++) {
            int p_rows = (p < remainder) ? (rows_per_proc + 1) : rows_per_proc;
            recvcounts[p] = p_rows * WIDTH;
            displs[p] = (p == 0) ? 0 : displs[p - 1] + recvcounts[p - 1];
        }
    }

    MPI_Gatherv(local_data, local_rows * WIDTH, MPI_UNSIGNED_LONG_LONG,
                global_data, recvcounts, displs, MPI_UNSIGNED_LONG_LONG,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("\nМінімальне значення шифротексту: %llu\n", global_min);
        printf("Максимальне значення шифротексту: %llu\n", global_max);
        printf("Час виконання: %f секунд\n", global_time);
        printf("Верхній лівий елемент: %llu\n", global_data[0]);
        printf("Верхній правий елемент: %llu\n", global_data[WIDTH - 1]);
        printf("Нижній лівий елемент: %llu\n", global_data[(HEIGHT - 1) * WIDTH]);
        printf("Нижній правий елемент: %llu\n", global_data[HEIGHT * WIDTH - 1]);
        printf("Центр: %llu\n", global_data[(HEIGHT / 2) * WIDTH + (WIDTH / 2)]);
    }

    free(local_data);
    if (rank == 0) {
        free(global_data);
        free(recvcounts);
        free(displs);
    }

    MPI_Finalize();
    return 0;
}
