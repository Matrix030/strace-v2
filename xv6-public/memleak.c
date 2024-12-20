#include "types.h"
#include "stat.h"
#include "user.h"

#define DATA_SIZE (sizeof(int) * 10000000)

int main() {
    char *data;
    int alloc_count = 0;

    while (1) {
        data = sbrk(DATA_SIZE);  // Allocate DATA_SIZE bytes using sbrk
        if (data == (char *)-1) {
            printf(1, "Memory allocation failed after %d allocations.\n", alloc_count);
            break;
        }
        memset(data, 0, DATA_SIZE);  // Optional: initialise memory
        alloc_count++;
    }

    printf(1, "Total successful allocations: %d\n", alloc_count);
    exit();
}