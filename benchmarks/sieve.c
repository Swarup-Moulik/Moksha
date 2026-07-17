#include <stdio.h>
#include <stdlib.h>

int main() {
    int N = 10000000;
    int *is_prime = (int *)malloc((N + 1) * sizeof(int));

    for (int i = 0; i <= N; i++) {
        is_prime[i] = 1;
    }

    is_prime[0] = 0;
    is_prime[1] = 0;

    for (int p = 2; p * p <= N; p++) {
        if (is_prime[p] == 1) {
            for (int i = p * p; i <= N; i += p) {
                is_prime[i] = 0;
            }
        }
    }

    int count = 0;
    for (int i = 2; i <= N; i++) {
        if (is_prime[i] == 1) {
            count++;
        }
    }

    printf("Primes up to %d: %d\n", N, count);
    free(is_prime);
    return 0;
}
