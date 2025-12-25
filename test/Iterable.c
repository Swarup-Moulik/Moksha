#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_STR_LEN 100
#define MAX_TABLE_SIZE 10
#define ITERATIONS 100000

// =====================
// DATA STRUCTURES
// =====================
typedef struct {
    char key[MAX_STR_LEN];
    char value[MAX_STR_LEN];
    bool active;
} TableEntry;


// =====================
// MAIN
// =====================
int main() {

    // =====================
    // 1. INPUT (ONCE)
    // =====================
    char w[MAX_STR_LEN];
    printf("\nEnter a string: ");
    scanf("%s", w);

    int size;
    printf("Enter array size: ");
    scanf("%d", &size);

    char** arInput = malloc(size * sizeof(char*));
    printf("Enter %d words for the array:\n", size);
    for (int i = 0; i < size; i++) {
        arInput[i] = malloc(MAX_STR_LEN);
        printf("ar[%d]: ", i);
        scanf("%s", arInput[i]);
    }

    char k[MAX_STR_LEN];
    char kValue[MAX_STR_LEN];

    printf("\nEnter a table key: ");
    scanf("%s", k);
    printf("Enter value for %s: ", k);
    scanf("%s", kValue);


    // =====================
    // 2. BENCHMARK LOOP
    // =====================
    for (int iter = 0; iter < ITERATIONS; iter++) {

        // --- fresh array copy ---
        char** ar = malloc(size * sizeof(char*));
        for (int i = 0; i < size; i++) {
            ar[i] = malloc(MAX_STR_LEN);
            strcpy(ar[i], arInput[i]);
        }

        // --- fresh table ---
        TableEntry tab[MAX_TABLE_SIZE] = {0};

        strcpy(tab[0].key, k);
        strcpy(tab[0].value, kValue);
        tab[0].active = true;

        strcpy(tab[1].key, "fixed");
        strcpy(tab[1].value, "unchangeable");
        tab[1].active = true;

        // --- Array loops ---
        for (int i = 0; i < size; i++) { }
        for (int i = 0; i < size; i++) { }
        for (int i = 0; i < size; i++) { }

        // --- Table loops ---
        for (int i = 0; i < MAX_TABLE_SIZE; i++) {
            if (tab[i].active) { }
        }

        for (int i = 0; i < MAX_TABLE_SIZE; i++) {
            if (tab[i].active) { }
        }

        // --- String iteration ---
        int wlen = strlen(w);
        if (wlen > 0) {
            for (int i = 0; i < wlen; i++) { }
        }

        // --- Complex assignment ---
        if (wlen > 3 && size > 3) {
            ar[3][0] = w[3];
            ar[3][1] = '\0';
        }

        if (size > 3) {
            strcpy(tab[2].key, "fromArray");
            strcpy(tab[2].value, ar[3]);
            tab[2].active = true;
        }

        int len = size;

        // --- Deletion (COMMENTED OUT) ---
        /*
        if (size > 3) {
            free(ar[3]);
            for (int i = 3; i < size - 1; i++) {
                ar[i] = ar[i + 1];
            }
            size--;
        }

        for (int i = 0; i < MAX_TABLE_SIZE; i++) {
            if (tab[i].active && strcmp(tab[i].key, k) == 0) {
                tab[i].active = false;
            }
        }
        */

        // --- cleanup per iteration ---
        for (int i = 0; i < size; i++) {
            free(ar[i]);
        }
        free(ar);
    }


    // =====================
    // 3. FINAL OUTPUT
    // =====================
    printf("\n=== TEST COMPLETED ===\n");
    printf("Iterations executed: %d\n", ITERATIONS);

    // --- cleanup input memory ---
    for (int i = 0; i < size; i++) free(arInput[i]);
    free(arInput);

    return 0;
}
