#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <error.h>
#include "allocator.h"

#define POOL_SIZE 1024 * 1024       // 1MB memory pool
#define MIN_BLOCK_SIZE 32
#define BLOCK_MAGIC 0xDEADBEEF
#define FREED_MAGIC 0xFEEDFACE
#define CANARY_VALUE 0xDEADC0DE
#define ALIGNMENT 8


typedef struct block_header {
    unsigned int magic;
    uint8_t is_free;
    uint8_t padding[3];         // 3 bytes explicit padding.
    size_t size;
    struct block_header* next;  // Pointer to next block in the list
} block_header_t;


static void* memory_pool = NULL;
static block_header_t* free_list_head = NULL;
static int initialized = 0; // False

size_t align_size(size_t size) {
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

void init_allocator() {
    if (!initialized) return;

    printf("[INIT] Requesting %zu bytes from OS via mmap()...\n", (size_t)POOL_SIZE);

    // Request memory from OS
    memory_pool = mmap(
        NULL,
        POOL_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if (memory_pool == MAP_FAILED) {
        perror("[ERROR] mmap failed");
        memory_pool = NULL;
        return;
    }

    printf("[INIT] Successfully allocated memory at %p\n", memory_pool);

    free_list_head = (block_header_t*)memory_pool;
    free_list_head->size = POOL_SIZE - sizeof(block_header_t);
    free_list_head->is_free = 1;
    free_list_head->next = NULL;
    free_list_head->magic = FREED_MAGIC;

    initialized = 1;
    printf("[INIT] Allocator initialized with %zu bytes\n", free_list_head->size);
}
