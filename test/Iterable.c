#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_STR_LEN 100
#define MAX_TABLE_SIZE 10

// 1. DATA STRUCTURES
typedef struct {
    char key[MAX_STR_LEN];
    char value[MAX_STR_LEN];
    bool active;
} TableEntry;

// --- HELPER FUNCTIONS ---

// Replicates "Pretty Print Array :- [val1, val2...]"
void pretty_print_array(char** ar, int size) {
    printf("[");
    for (int i = 0; i < size; i++) {
        printf("%s", ar[i]);
        if (i < size - 1) printf(", ");
    }
    printf("]");
}

// Replicates "Table : {key1: val1, key2: val2...}"
void pretty_print_table(TableEntry* tab) {
    printf("{");
    bool first = true;
    for (int i = 0; i < MAX_TABLE_SIZE; i++) {
        if (tab[i].active) {
            if (!first) printf(", ");
            printf("%s: %s", tab[i].key, tab[i].value);
            first = false;
        }
    }
    printf("}");
}

int main() {
    // --- 1. INPUT --- 
    char w[MAX_STR_LEN];
    printf("\nEnter a string: ");
    scanf("%s", w);

    int size;
    printf("Enter array size: ");
    scanf("%d", &size);

    char** ar = (char**)malloc(size * sizeof(char*));
    printf("Enter %d words for the array:\n", size); 
    for (int i = 0; i < size; i++) { 
        ar[i] = (char*)malloc(MAX_STR_LEN * sizeof(char));
        printf("ar[%d]: ", i);
        scanf("%s", ar[i]); 
    }

    TableEntry tab[MAX_TABLE_SIZE];
    for (int i = 0; i < MAX_TABLE_SIZE; i++) tab[i].active = false;

    char k[MAX_STR_LEN];
    printf("\nEnter a table key: ");
    scanf("%s", k);
    printf("Enter value for %s: ", k);
    
    // Assign age/custom key
    strcpy(tab[0].key, k);
    scanf("%s", tab[0].value);
    tab[0].active = true;

    // Assign fixed key
    strcpy(tab[1].key, "fixed");
    strcpy(tab[1].value, "unchangeable");
    tab[1].active = true;

    // --- 2. DISPLAY LOOPS --- 
    printf("\n--- Loop Display ---\n");
    printf("Pretty Print Array :- ");
    pretty_print_array(ar, size);
    printf("\n");

    printf("\nArray (For-In Value Only):\n"); 
    for (int i = 0; i < size; i++) {
        printf("%s\n", ar[i]);
    }

    printf("\nArray (For-In Index & Value):\n"); 
    for (int i = 0; i < size; i++) {
        printf("[%d] = %s\n", i, ar[i]); 
    }

    printf("\nArray (For-In Value):\n");
    for (int i = 0; i < size; i++) {
        printf("Element = %s\n", ar[i]); 
    }

    printf("\nTable (Key, Value):\n");
    for (int i = MAX_TABLE_SIZE - 1; i >= 0; i--) { // Reverse to match output order
        if (tab[i].active) {
            printf("Key: %s, Val: %s\n", tab[i].key, tab[i].value); 
        }
    }

    printf("\nTable (Key):\n");
    for (int i = MAX_TABLE_SIZE - 1; i >= 0; i--) {
        if (tab[i].active) {
            printf("Key: %s\n", tab[i].key); 
        }
    }

    printf("\nString (For-In):\n");
    if (strlen(w) > 0) { 
        for (int i = 0; i < strlen(w); i++) {
            printf("%c\n", w[i]); 
        }
    }

    // --- 3. COMPLEX ASSIGNMENT ---
    printf("\n--- Complex Assignment ---\n");

    if (strlen(w) > 3) {
        printf("4th letter of string: %c\n", w[3]);
        if (size > 3) { 
            char temp[2] = {w[3], '\0'};
            strcpy(ar[3], temp);
            printf("Assigned ar[3] = %s\n", ar[3]); 
        }
    }

    if (size > 3) {
        strcpy(tab[2].key, "fromArray");
        strcpy(tab[2].value, ar[3]);
        tab[2].active = true;
        printf("Assigned tab['fromArray'] = %s\n", ar[3]); 
    }

    printf("Table : ");
    pretty_print_table(tab);
    printf("\nLength of array : %d\n", size); 

    // --- 4. DELETION ---
    printf("\n--- Deletion ---\n"); 
    if (size > 3) {
        printf("Deleting ar[3] (%s)...\n", ar[3]);
        free(ar[3]);
        // Shift elements left to "delete"
        for (int i = 3; i < size - 1; i++) {
            ar[i] = ar[i + 1];
        }
        size--;
        printf("Array after delete: ");
        pretty_print_array(ar, size); 
        printf("\n");
    }

    printf("Length of array : %d\n", size);

    printf("Deleting tab[%s]...\n", k);
    for (int i = 0; i < MAX_TABLE_SIZE; i++) {
        if (strcmp(tab[i].key, k) == 0) {
            tab[i].active = false;
        }
    }
    printf("Table after delete: ");
    pretty_print_table(tab); 
    printf("\n");

    printf("\n=== TEST COMPLETED ===\n");

    // Cleanup memory
    for (int i = 0; i < size; i++) free(ar[i]);
    free(ar);

    return 0;
}
