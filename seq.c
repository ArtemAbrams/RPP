#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <time.h>

typedef unsigned long long ull;
typedef long long ll;

#define WIDTH   3000
#define HEIGHT  3000

const ll p_const = 3000000007LL;
const ll q_const = 3000000011LL;
const ll n_const = p_const * q_const;
const ll e_const = 90000000000000LL;

ull modexp(ull base, ll exp, ull mod) {
    ull result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1) result = (result * base) % mod;
        exp >>= 1;
        base = (base * base) % mod;
    }
    return result;
}

int main(void) {
    int i, j;

    ull *data = malloc(WIDTH * HEIGHT * sizeof(ull));
    if (!data) {
        fprintf(stderr, "Помилка виділення пам'яті!\n");
        return 1;
    }

    ull global_min = ULLONG_MAX;
    ull global_max = 0;

    clock_t t0 = clock();

    for (i = 0; i < HEIGHT; i++) {
        for (j = 0; j < WIDTH; j++) {
            ll message = (ll)i * WIDTH + j;
            ull ciphertext = modexp(message, e_const, n_const);
            data[i * WIDTH + j] = ciphertext;

            if (ciphertext < global_min) global_min = ciphertext;
            if (ciphertext > global_max) global_max = ciphertext;
        }
    }

    double elapsed = (double)(clock() - t0) / CLOCKS_PER_SEC;
    printf("Згенеровано за %.3f с. min = %llu, max = %llu\n",
           elapsed, global_min, global_max);

    printf("Верхній лівий:   %llu\n", data[0]);
    printf("Верхній правий:  %llu\n", data[WIDTH - 1]);
    printf("Нижній лівий:    %llu\n", data[(HEIGHT - 1) * WIDTH]);
    printf("Нижній правий:   %llu\n", data[HEIGHT * WIDTH - 1]);
    printf("Центр:           %llu\n",
           data[(HEIGHT / 2) * WIDTH + (WIDTH / 2)]);

    free(data);
    return 0;
}
