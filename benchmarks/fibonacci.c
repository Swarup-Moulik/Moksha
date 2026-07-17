#include <stdio.h>

int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    int n = 42;
    int result = fibonacci(n);
    printf("Fibonacci(%d) = %d\n", n, result);
    return 0;
}
