#include "tiny_alloc.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#define TALLOC_MAX_HEAP_SIZE (1024*1024*4) // 4 mebibyte
#define TALLOC_MAX_HEAP_CHUNKS 4096
#define TALLOC_USE_STATIC 0

typedef struct heap_info_t {
#if TALLOC_USE_STATIC
    char heapPointer[TALLOC_MAX_HEAP_SIZE];
#else
    char* heapPointer;
#endif
    bool initialized;
} heap_info;

typedef char chunk_state;


typedef struct heap_chunk_t {
    void* pointer;
    size_t count;
    bool isFree;
    struct heap_chunk_t* next;
    struct heap_chunk_t* prev;
} heap_chunk;

static heap_info mainHeapInfo = {};
static heap_chunk chunks[TALLOC_MAX_HEAP_CHUNKS] = {0};
static size_t chunksCount = 0;
static heap_chunk* hollowChunks[TALLOC_MAX_HEAP_CHUNKS] = {0};
static size_t hollowChunksCount = 0;
static heap_chunk* head;

void initialize_chunks() {
    chunks[0].count = TALLOC_MAX_HEAP_SIZE;
    chunks[0].pointer = (void*)mainHeapInfo.heapPointer;
    chunks[0].isFree = true;
    chunks[0].prev = 0;
    chunks[0].next = 0;
    chunksCount = 1;
    head = &chunks[0];
    for (size_t i = 1; i < TALLOC_MAX_HEAP_CHUNKS; ++i) {
        hollowChunks[i] = &chunks[i]; 
    }
    hollowChunksCount = TALLOC_MAX_HEAP_CHUNKS - 1;
}
#if TALLOC_USE_STATIC
void initialize_heap() {
    mainHeapInfo.initialized = true;
    initialize_chunks();
}
#else
#ifdef __linux__
#include <unistd.h>
#include <sys/mman.h>
void initialize_heap() {
    mainHeapInfo.heapPointer = (char*)mmap(0, TALLOC_MAX_HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    mainHeapInfo.initialized = mainHeapInfo.heapPointer != MAP_FAILED;
    assert(mainHeapInfo.initialized);
    initialize_chunks();
}
#elif (defined __WIN32)
#include <windows.h>
void initialize_heap() {
    mainHeapInfo.heapPointer = (char*)VirtualAlloc(0, TALLOC_MAX_HEAP_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    mainHeapInfo.initialized = mainHeapInfo.heapPointer != 0;
    assert(mainHeapInfo.initialized);
    initialize_chunks();
}
#else
static_assert(false, "not supported");
#endif // linux or windows
#endif // TALLOC_USE_STATIC

void* talloc(size_t count) {
    if ((chunksCount > TALLOC_MAX_HEAP_CHUNKS) || (count == 0))
        return 0;
    if (!mainHeapInfo.initialized)
        initialize_heap();
    heap_chunk* current = head;
    while (current != 0) {
        if ((current->isFree) && (current->count >= count)) {
            if (current->next == 0) { // is a tale
                heap_chunk* next = current->next = hollowChunks[hollowChunksCount - 1];
                --hollowChunksCount;
                next->count = count;
                next->prev = current;
                next->next = 0;
                next->isFree = false;

                next->pointer = current->pointer + current->count - count;
                current->count -= count;
                
                if (current->count == 0) {
                    hollowChunks[hollowChunksCount] = current;
                    ++hollowChunksCount;
                    if (current->prev != 0)
                        current->prev->next = 0;
                    if (current == head) {
                        head = current->next;
                        head->prev = 0;
                    };
                } else {
                    ++chunksCount;
                }
                return next->pointer;
            } else {
                heap_chunk* newNext = hollowChunks[hollowChunksCount - 1];
                --hollowChunksCount;
                heap_chunk* curNext = current->next;
                newNext->next = curNext;
                newNext->prev = current;
                current->next = newNext;
                curNext->prev = newNext;

                newNext->isFree = false;
                newNext->count = count;
                newNext->pointer = current->pointer + current->count - count;
                current->count -= count;
                if (current->count == 0) {
                    hollowChunks[hollowChunksCount] = current;
                    ++hollowChunksCount;
                    if (current->prev != 0)
                        current->prev->next = newNext;
                    if (current == head) {
                        head = current->next;
                        head->prev = 0;
                    }
                } else {
                    ++chunksCount;
                }
                return newNext->pointer;
            }
        }
        current = current->next;
    }
    return 0;
}

void tfree(void* pointer) {
    heap_chunk* current = head;
    while (current != 0) {
        if (current->pointer == pointer) {
            current->isFree = true;
            heap_chunk* prev = current->prev;
            heap_chunk* next = current->next;
            const int mask = 
                ((next != 0) && (next->isFree) ? 1 : 0) |
                ((prev != 0) && (prev->isFree) ? 2 : 0);
            switch (mask) {
            case 1: {
                current->count += next->count;
                current->next = next->next;
                if (next->next != 0)
                    next->next->prev = current;
                hollowChunks[hollowChunksCount] = next;
                ++hollowChunksCount;
                --chunksCount;
                break;
            } 
            case 2: {
                prev->count += current->count;
                prev->next = next;
                if (next != 0)
                    next->prev = prev;
                hollowChunks[hollowChunksCount] = current;
                ++hollowChunksCount;
                --chunksCount;
                break;
            } 
            case 3: {
                prev->count += current->count + next->count;
                prev->next = next->next;
                if (next->next != 0)
                    next->next->prev = prev;
                hollowChunks[hollowChunksCount] = current;
                hollowChunks[hollowChunksCount + 1] = next;
                hollowChunksCount += 2;
                chunksCount -= 2;
                break;
            }
            default:
                break;
            }
            break;
        }
        current = current->next;
    }
}
void heap_view() {
    if (!mainHeapInfo.initialized)
        initialize_heap();
    heap_chunk* current = head;
    printf("chunks: %zu, hollow chunks: %zu\n", chunksCount, hollowChunksCount);
    while (current != 0) {
        if (current->isFree)
            printf("free: %p: %zu\n", current->pointer, current->count);
        else
            printf("commited: %p: %zu\n", current->pointer, current->count);
        current = current->next;
    }
}