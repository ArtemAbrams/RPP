#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define WIDTH 3000
#define HEIGHT 3000

const long long p_const = 3000000007LL;
const long long q_const = 3000000011LL;
const long long n_const = p_const * q_const;
const long long phi_const = (p_const - 1) * (q_const - 1);
const long long e_const = 900000000000000000LL;

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
    int i, j;

    clock_t start_time = clock();

    long long total = 0;
    int global_min = n_const;
    int global_max = 0;

    int *image = malloc(WIDTH * HEIGHT * sizeof(int));
    if (image == NULL) {
        fprintf(stderr, "Помилка виділення пам'яті для зображення!\n");
        return 1;
    }

    for (i = 0; i < HEIGHT; i++) {
        for (j = 0; j < WIDTH; j++) {
            int message = (i * WIDTH + j) % n_const;
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
            printf("Оброблено рядок %d\n", i);
        }
    }

    // Зафіксуємо час завершення обчислень
    clock_t end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    printf("Зображення згенеровано за %f секунд. Total = %lld, min = %d, max = %d\n",
           elapsed_time, total, global_min, global_max);

    // Запис зображення у файл (формат PPM, формат P6)
    char filename[256];
    sprintf(filename, "Sequential-RSA.ppm");

    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Помилка відкриття файлу %s для запису зображення!\n", filename);
        free(image);
        return 1;
    }
    fprintf(fp, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
    // Масштабування значення шифротексту до відтінків сірого (діапазон 0-255)
    for (i = 0; i < HEIGHT; i++) {
        for (j = 0; j < WIDTH; j++) {
            int ciphertext = image[i * WIDTH + j];
            unsigned char color = (unsigned char)(255 * ciphertext / (double)(n_const - 1));
            fwrite(&color, 1, 1, fp);
            fwrite(&color, 1, 1, fp);
            fwrite(&color, 1, 1, fp);
        }
    }
    fclose(fp);

    printf("Зображення збережено у файлі %s\n", filename);
    free(image);

    return 0;
}
