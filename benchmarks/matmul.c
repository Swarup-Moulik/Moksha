#include <stdio.h>
#include <stdlib.h>

int main() {
    int N = 1024;
    float *A = (float *)malloc(N * N * sizeof(float));
    float *B = (float *)malloc(N * N * sizeof(float));
    float *C = (float *)malloc(N * N * sizeof(float));

    for (int i = 0; i < N * N; i++) {
        A[i] = 1.0f;
        B[i] = 2.0f;
        C[i] = 0.0f;
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < N; k++) {
                sum += A[i * N + k] * B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }

    printf("MatMul Complete. C[0] = %f\n", C[0]);

    free(A);
    free(B);
    free(C);
    return 0;
}
