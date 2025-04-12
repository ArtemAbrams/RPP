#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define NUM_FRAMES 10  // Кількість кадрів для анімації

// Вхідні параметри, які задаються root-процесом і передаються через MPI_Bcast
int WIDTH, HEIGHT, MAX_ITER;
double real_min, real_max, imag_min, imag_max;

// RSA параметри для демонстрації (небезпечні для реальної криптографії)
// Обираємо прості числа порядку ~3×10⁹.
const long long p_const = 3000000007LL;
const long long q_const = 3000000011LL;
// Модуль RSA: n ≈ 9×10^18 (має залишатися в межах типу long long)
const long long n_const = p_const * q_const;
// φ(n) = (p-1)*(q-1) (не використовується в цьому прикладі)
const long long phi_const = (p_const - 1) * (q_const - 1);
// Дуже великий відкритий експонент для шифрування (для уповільнення обчислень)
const long long e_const = 900000000000000000LL;

// Функція для обчислення модульного піднесення до степеня (RSA‑шифрування)
long long modexp(long long base, long long exp, long long mod) {
    long long result = 1;
    base = base % mod;
    while (exp > 0) {
        if (exp % 2 == 1)
            result = (result * base) % mod;
        exp = exp / 2;
        base = (base * base) % mod;
    }
    return result;
}

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);                           // Ініціалізація MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);             // Отримання номера процесу
    MPI_Comm_size(MPI_COMM_WORLD, &size);             // Отримання загальної кількості процесів

    // Root-процес задає вхідні параметри
    if (rank == 0) {
        WIDTH = 3000;
        HEIGHT = 3000;
        MAX_ITER = 1000;  // Будемо використовувати це значення для масштабування (як "максимальне" значення)
        real_min = -2.0;
        real_max = 1.0;
        imag_min = -1.5;
        imag_max = 1.5;
    }

    // Передаємо параметри всім процесам
    MPI_Bcast(&WIDTH, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&HEIGHT, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&MAX_ITER, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&real_min, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&real_max, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&imag_min, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&imag_max, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Параметри для динамічної візуалізації (zoom‑ефект);
    // хоча в RSA-логіці вони не використовуються, ми їх залишаємо для збереження структури.
    double center_real = -0.5, center_imag = 0.0;
    double initial_width = real_max - real_min;
    double initial_height = imag_max - imag_min;
    double zoom_step = 1.05;

    // Цикл для генерації NUM_FRAMES кадрів
    for (int frame = 0; frame < NUM_FRAMES; frame++) {

        // Динамічне збільшення "максимальної" кількості ітерацій (використовується для масштабування кольору)
        int current_max_iter = ((frame + 1) * MAX_ITER) / NUM_FRAMES;
        if (current_max_iter < 1)
            current_max_iter = 1;

        // Обчислюємо коефіцієнт zoom та оновлюємо межі (цей розрахунок не впливає на RSA-логіку, але збережено для структури)
        double zoom = pow(zoom_step, frame);
        double current_width = initial_width / zoom;
        double current_height = initial_height / zoom;
        real_min = center_real - current_width / 2;
        real_max = center_real + current_width / 2;
        imag_min = center_imag - current_height / 2;
        imag_max = center_imag + current_height / 2;

        // Розподіл рядків зображення між процесами
        int rows_per_proc = HEIGHT / size;
        int remainder = HEIGHT % size;
        int start_row, end_row;
        if (rank < remainder) {
            start_row = rank * (rows_per_proc + 1);
            end_row = start_row + rows_per_proc + 1;
        } else {
            start_row = rank * rows_per_proc + remainder;
            end_row = start_row + rows_per_proc;
        }
        int local_rows = end_row - start_row;

        // Виділення пам'яті для локального блоку зображення; кожен процес зберігає результати для своїх рядків
        int *local_image = malloc(local_rows * WIDTH * sizeof(int));
        if (!local_image) {
            fprintf(stderr, "Процес %d: Помилка виділення пам'яті для local_image\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // Локальні статистики: сума, мінімум та максимум значень (RSA‑шифротекстів)
        long long local_total = 0;
        int local_min = current_max_iter;  // Використовуємо current_max_iter як початкове значення
        int local_max = 0;

        // Фіксуємо час початку локальних обчислень
        double start_time = MPI_Wtime();

        for (int i = 0; i < local_rows; i++) {
            int global_row = start_row + i;
            if (global_row % 500 == 0) {
                printf("Процес %d: обробка глобального рядка %d (Frame %d, current_max_iter = %d)\n", rank, global_row, frame, current_max_iter);
            }
            for (int j = 0; j < WIDTH; j++) {
                // Формуємо повідомлення як функцію від глобальної позиції та номера кадру.
                int message = (global_row * WIDTH + j + frame) % n_const;
                // RSA‑шифрування: ciphertext = message^e mod n
                long long ciphertext = modexp(message, e_const, n_const);
                // Зберігаємо результат (конвертуємо до int; для демонстрації, переповнення може бути)
                local_image[i * WIDTH + j] = (int)ciphertext;
                local_total += ciphertext;
                if (ciphertext < local_min)
                    local_min = (int)ciphertext;
                if (ciphertext > local_max)
                    local_max = (int)ciphertext;
            }
        }

        double end_time = MPI_Wtime();
        double local_time = end_time - start_time;
        double global_time;
        MPI_Reduce(&local_time, &global_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        long long global_total;
        int global_min_val, global_max_val;
        MPI_Reduce(&local_total, &global_total, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&local_min, &global_min_val, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
        MPI_Reduce(&local_max, &global_max_val, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

        // Збір локальних блоків у глобальний масив на root-процесі.
        int *global_image = NULL;
        int *recvcounts = NULL;
        int *displs = NULL;
        if (rank == 0) {
            global_image = malloc(HEIGHT * WIDTH * sizeof(int));
            recvcounts = malloc(size * sizeof(int));
            displs = malloc(size * sizeof(int));
            int offset = 0;
            for (int p = 0; p < size; p++) {
                int p_rows = (p < remainder) ? (rows_per_proc + 1) : rows_per_proc;
                recvcounts[p] = p_rows * WIDTH;
                displs[p] = offset;
                offset += recvcounts[p];
            }
        } else {
            global_image = malloc(HEIGHT * WIDTH * sizeof(int));
        }

        MPI_Gatherv(local_image, local_rows * WIDTH, MPI_INT,
                    global_image, recvcounts, displs, MPI_INT,
                    0, MPI_COMM_WORLD);

        // На root-процесі записуємо кадр у файл формату PPM.
        if (rank == 0) {
            printf("\nMPI RSA: Frame %d\n", frame);
            printf("Глобальна сума = %lld, глобально мін = %d, глобально макс = %d, час = %f секунд\n",
                   global_total, global_min_val, global_max_val, global_time);
            printf("Зразкові значення: Верхній лівий = %d, Верхній правий = %d, Нижній лівий = %d, Нижній правий = %d, Центр = %d\n",
                   global_image[0],
                   global_image[WIDTH - 1],
                   global_image[(HEIGHT - 1) * WIDTH],
                   global_image[HEIGHT * WIDTH - 1],
                   global_image[(HEIGHT / 2) * WIDTH + (WIDTH / 2)]);

            char filename[256];
            sprintf(filename, "MPI-frame_%04d.ppm", frame);
            FILE *fp = fopen(filename, "wb");
            if (fp == NULL) {
                fprintf(stderr, "Помилка відкриття файлу %s для запису зображення!\n", filename);
                free(global_image);
                free(recvcounts);
                free(displs);
                MPI_Finalize();
                return 1;
            }
            // Запис заголовку файлу PPM: формат P6, розміри зображення та максимальне значення кольору (255)
            fprintf(fp, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
            // Масштабування значень RSA‑шифротекстів (local_image) до відтінків сірого,
            // використовуючи current_max_iter для масштабування кольору.
            for (int i = 0; i < HEIGHT; i++) {
                for (int j = 0; j < WIDTH; j++) {
                    int val = global_image[i * WIDTH + j];
                    unsigned char color = (unsigned char)(255 * val / (double)(current_max_iter - 1));
                    fwrite(&color, 1, 1, fp);
                    fwrite(&color, 1, 1, fp);
                    fwrite(&color, 1, 1, fp);
                }
            }
            fclose(fp);
            free(global_image);
            free(recvcounts);
            free(displs);
        } else {
            free(global_image);
        }
        free(local_image);
    }

    // Після генерації всіх кадрів root-процес об'єднує їх у анімований GIF за допомогою ImageMagick.
    if (rank == 0) {
        system("convert -delay 20 -loop 0 MPI-frame_*.ppm MPI-RSA-animation.gif");
        printf("Animated GIF generated as MPI-RSA-animation.gif\n");
    }

    MPI_Finalize();
    return 0;
}
