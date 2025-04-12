#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

#define WIDTH 3000       // Ширина "зображення"
#define HEIGHT 3000      // Висота "зображення"
#define NUM_FRAMES 10    // Кількість кадрів для анімації

// Збільшені RSA параметри (демонстраційні; для реальної криптографії слід використовувати бібліотеки для великих чисел)
// Обираємо прості числа порядку 3×10⁹.
const long long p_const = 3000000007LL;
const long long q_const = 3000000011LL;
// Обчислення n = p * q; очікуване значення ≈ 9×10^18 (зверніть увагу, що воно має залишатися в межах long long)
const long long n_const = p_const * q_const;
// φ(n) = (p-1)*(q-1) (не використовується в даному прикладі)
const long long phi_const = (p_const - 1) * (q_const - 1);
// Встановлюємо дуже великий експонент для шифрування, що значно збільшує кількість ітерацій у алгоритмі modular exponentiation
const long long e_const = 900000000000000000LL;

// Функція для обчислення модульного піднесення до степеня (експонціація за модулем)
// Увага: використання дуже великих чисел може призвести до переповнення стандартного 64-бітного типу при множенні.
// Це демонстраційний приклад – для роботи з надвеликими числами слід використовувати тип __int128 або спеціалізовані бібліотеки.
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

int main(void) {
    int frame, i, j;

    // Генеруємо NUM_FRAMES кадрів
    for (frame = 0; frame < NUM_FRAMES; frame++) {
        // Використовуємо значення n_const для масштабування кольору: максимальне значення ciphertext буде n_const - 1.
        int current_max_iter = n_const;

        // Глобальні змінні для статистики
        long long total = 0;
        int global_min = current_max_iter;
        int global_max = 0;

        // Виділення пам'яті для збереження результатів для кожного "пікселя"
        int *image = malloc(WIDTH * HEIGHT * sizeof(int));
        if (image == NULL) {
            fprintf(stderr, "Помилка виділення пам'яті для кадру %d!\n", frame);
            return 1;
        }

        double start_time = omp_get_wtime();

        int max_threads = omp_get_max_threads();
        int *rows_processed = calloc(max_threads, sizeof(int));
        if (rows_processed == NULL) {
            fprintf(stderr, "Помилка виділення пам'яті для rows_processed в кадрі %d!\n", frame);
            free(image);
            return 1;
        }

        // Паралельна область: для кожного "пікселя" виконуємо RSA-шифрування
#pragma omp parallel for private(j) reduction(+:total) reduction(min:global_min) reduction(max:global_max) schedule(dynamic)
        for (i = 0; i < HEIGHT; i++) {
            int tid = omp_get_thread_num();
            rows_processed[tid]++;
            for (j = 0; j < WIDTH; j++) {
                // Повідомлення обчислюється на основі позиції та номера кадру
                int message = (i * WIDTH + j + frame) % n_const;
                // RSA-шифрування: ciphertext = message^e mod n
                long long ciphertext = modexp(message, e_const, n_const);
                image[i * WIDTH + j] = (int)ciphertext;

                total += ciphertext;
                if (ciphertext < global_min)
                    global_min = (int)ciphertext;
                if (ciphertext > global_max)
                    global_max = (int)ciphertext;
            }
            if (i % 500 == 0) {
                printf("Frame %d, Thread %d: оброблено рядок %d\n", frame, tid, i);
            }
        }

        double end_time = omp_get_wtime();
        printf("Frame %d generated in %f секунд. total = %lld, min = %d, max = %d\n",
               frame, end_time - start_time, total, global_min, global_max);
        for (i = 0; i < max_threads; i++) {
            printf("Frame %d: Thread %d обробив %d рядків\n", frame, i, rows_processed[i]);
        }
        free(rows_processed);

        // Формування імені файлу для поточного кадру
        char filename[256];
        sprintf(filename, "OpenMP-frame_%04d.ppm", frame);

        // Запис зображення у форматі PPM (формат P6)
        FILE *fp = fopen(filename, "wb");
        if (fp == NULL) {
            fprintf(stderr, "Помилка відкриття файлу %s для запису зображення!\n", filename);
            free(image);
            return 1;
        }
        fprintf(fp, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
        for (i = 0; i < HEIGHT; i++) {
            for (j = 0; j < WIDTH; j++) {
                int ciphertext = image[i * WIDTH + j];
                unsigned char color = (unsigned char)(255 * ciphertext / (double)(current_max_iter - 1));
                fwrite(&color, 1, 1, fp);
                fwrite(&color, 1, 1, fp);
                fwrite(&color, 1, 1, fp);
            }
        }
        fclose(fp);

        // Вивід статистики для поточного кадру
        printf("\nRSA: Кадр %d\n", frame);
        printf("Контрольна сума (сума шифротекстів): %lld\n", total);
        printf("Мінімальне значення шифротексту: %d\n", global_min);
        printf("Максимальне значення шифротексту: %d\n", global_max);
        printf("Час виконання: %f секунд\n", end_time - start_time);
        printf("Зразкові значення:\n");
        printf("Верхній лівий елемент: %d\n", image[0]);
        printf("Верхній правий елемент: %d\n", image[WIDTH - 1]);
        printf("Нижній лівий елемент: %d\n", image[(HEIGHT - 1) * WIDTH]);
        printf("Нижній правий елемент: %d\n", image[HEIGHT * WIDTH - 1]);
        printf("Центр: %d\n", image[(HEIGHT / 2) * WIDTH + (WIDTH / 2)]);

        free(image);
    }

    // Об'єднання усіх кадрів в анімований GIF за допомогою ImageMagick
    system("convert -delay 20 -loop 0 OpenMP-frame_*.ppm OpenMP-RSA-animation.gif");
    printf("Animated GIF generated as OpenMP-RSA-animation.gif\n");

    return 0;
}
