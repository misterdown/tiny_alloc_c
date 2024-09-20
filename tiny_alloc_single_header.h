#ifndef TINY_ALLOC_SINGLE_HEADER_H_

#include <stdio.h>
#ifndef TALLOC_ASSERT
#   include <assert.h>
#   define TALLOC_ASSERT(expr__) assert(expr__)
#endif
#ifndef TALLOC_SIZE_TYPE
#   include <stdint.h>
#   define TALLOC_SIZE_TYPE size_t
#endif
#ifndef TALLOC_BOOL
#   include <stdbool.h>
#   define TALLOC_BOOL bool
#endif
#ifndef TALLOC_FALSE
#   include <stdbool.h>
#   define TALLOC_FALSE false
#endif
#ifndef TALLOC_TRUE
#   include <stdbool.h>
#   define TALLOC_TRUE true
#endif
#ifndef TALLOC_DEF
#   define TALLOC_DEF extern
#endif

/// @brief A function that allocates an amount of memory equal to count. Returns 0 if the function fails for some reason.
///        If this is the first call to this function, it initializes the heap, and an assert is triggered in case of failure.
/// @param count count of bytes to allocate.
/// @return Valid or zero pointer.
TALLOC_DEF void* talloc(TALLOC_SIZE_TYPE count);
/// @brief Deallocates the memory allocated for `pointer` `talloc`. If for some reason the function cannot free the memory for this pointer, it does nothing. @param pointer pointer to free.
TALLOC_DEF void tfree(void* pointer);
#ifdef TALLOC_TESTING
/// @brief Prints to stdout basic information about the heap and chunks used for the operation of the `talloc` and `tfree` functions. If this is the first call to this function, it initializes the heap, and an assert is triggered in case of failure.
TALLOC_DEF void heap_view();
#endif

#ifdef TALLOC_IMPLEMENTATION
#ifndef TALLOC_MAX_HEAP_SIZE
#   define TALLOC_MAX_HEAP_SIZE (1024*1024*4) // 4 mebibytes
#endif

#ifndef TALLOC_MAX_HEAP_CHUNKS
#   define TALLOC_MAX_HEAP_CHUNKS 4096
#endif

#ifndef TALLOC_USE_STATIC
#   define TALLOC_USE_STATIC 0
#endif

typedef struct heap_info_t {
#if TALLOC_USE_STATIC
    char heapPointer[TALLOC_MAX_HEAP_SIZE];
#else
    char* heapPointer;
#endif
    TALLOC_BOOL initialized;
} heap_info;

typedef char chunk_state;


typedef struct heap_chunk_t {
    void* pointer;
    TALLOC_SIZE_TYPE count;
    TALLOC_BOOL isFree;
    struct heap_chunk_t* next;
    struct heap_chunk_t* prev;
} heap_chunk;

static heap_info tallocMainHeapInfo = {};
static heap_chunk tallocChunks[TALLOC_MAX_HEAP_CHUNKS] = {0};
static TALLOC_SIZE_TYPE chunksCount = 0;
static heap_chunk* tallocHollowChunks[TALLOC_MAX_HEAP_CHUNKS] = {0};
static TALLOC_SIZE_TYPE tallocHollowChunksCount = 0;
static heap_chunk* tallocHead;

void talloc_initialize_chunks() {
    tallocChunks[0].count = TALLOC_MAX_HEAP_SIZE;
    tallocChunks[0].pointer = (void*)tallocMainHeapInfo.heapPointer;
    tallocChunks[0].isFree = TALLOC_TRUE;
    tallocChunks[0].prev = 0;
    tallocChunks[0].next = 0;
    chunksCount = 1;
    tallocHead = &tallocChunks[0];
    for (TALLOC_SIZE_TYPE i = 1; i < TALLOC_MAX_HEAP_CHUNKS; ++i) {
        tallocHollowChunks[i] = &tallocChunks[i]; 
    }
    tallocHollowChunksCount = TALLOC_MAX_HEAP_CHUNKS - 1;
}
#if TALLOC_USE_STATIC
void talloc_initialize_heap() {
    tallocMainHeapInfo.initialized = TALLOC_TRUE;
    talloc_initialize_chunks();
}
#else
#ifdef __linux__
#include <unistd.h>
#include <sys/mman.h>
void talloc_initialize_heap() {
    tallocMainHeapInfo.heapPointer = (char*)mmap(0, TALLOC_MAX_HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    tallocMainHeapInfo.initialized = tallocMainHeapInfo.heapPointer != MAP_FAILED;
    TALLOC_ASSERT(tallocMainHeapInfo.initialized);
    talloc_initialize_chunks();
}
#elif (defined __WIN32)
#include <windows.h>
void talloc_initialize_heap() {
    tallocMainHeapInfo.heapPointer = (char*)VirtualAlloc(0, TALLOC_MAX_HEAP_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    tallocMainHeapInfo.initialized = tallocMainHeapInfo.heapPointer != 0;
    TALLOC_ASSERT(tallocMainHeapInfo.initialized);
    talloc_initialize_chunks();
}
#else
static_assert(0, "not supported");
#endif // linux or windows
#endif // TALLOC_USE_STATIC

TALLOC_DEF void* talloc(TALLOC_SIZE_TYPE count) {
    if ((chunksCount > TALLOC_MAX_HEAP_CHUNKS) || (count == 0))
        return 0;
    if (!tallocMainHeapInfo.initialized)
        talloc_initialize_heap();
    heap_chunk* current = tallocHead;
    while (current != 0) {
        if ((current->isFree) && (current->count >= count)) {
            if (current->next == 0) { // is a tale
                heap_chunk* next = current->next = tallocHollowChunks[tallocHollowChunksCount - 1];
                --tallocHollowChunksCount;
                next->count = count;
                next->prev = current;
                next->next = 0;
                next->isFree = TALLOC_FALSE;

                next->pointer = current->pointer + current->count - count;
                current->count -= count;
                
                if (current->count == 0) {
                    tallocHollowChunks[tallocHollowChunksCount] = current;
                    ++tallocHollowChunksCount;
                    if (current->prev != 0)
                        current->prev->next = 0;
                    if (current == tallocHead) {
                        tallocHead = current->next;
                        tallocHead->prev = 0;
                    };
                } else {
                    ++chunksCount;
                }
                return next->pointer;
            } else {
                heap_chunk* newNext = tallocHollowChunks[tallocHollowChunksCount - 1];
                --tallocHollowChunksCount;
                heap_chunk* curNext = current->next;
                newNext->next = curNext;
                newNext->prev = current;
                current->next = newNext;
                curNext->prev = newNext;

                newNext->isFree = TALLOC_FALSE;
                newNext->count = count;
                newNext->pointer = current->pointer + current->count - count;
                current->count -= count;
                if (current->count == 0) {
                    tallocHollowChunks[tallocHollowChunksCount] = current;
                    ++tallocHollowChunksCount;
                    if (current->prev != 0)
                        current->prev->next = newNext;
                    if (current == tallocHead) {
                        tallocHead = current->next;
                        tallocHead->prev = 0;
                    }
                } else {
                    ++chunksCount;
                }
                return newNext->pointer;
            }
        }
        current = current->next;
    }
    printf("%zu", count);
    return 0;
}
TALLOC_DEF void tfree(void* pointer) {
    heap_chunk* current = tallocHead;
    while (current != 0) {
        if (current->pointer == pointer) {
            current->isFree = TALLOC_TRUE;
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
                tallocHollowChunks[tallocHollowChunksCount] = next;
                ++tallocHollowChunksCount;
                --chunksCount;
                break;
            } 
            case 2: {
                prev->count += current->count;
                prev->next = next;
                if (next != 0)
                    next->prev = prev;
                tallocHollowChunks[tallocHollowChunksCount] = current;
                ++tallocHollowChunksCount;
                --chunksCount;
                break;
            } 
            case 3: {
                prev->count += current->count + next->count;
                prev->next = next->next;
                if (next->next != 0)
                    next->next->prev = prev;
                tallocHollowChunks[tallocHollowChunksCount] = current;
                tallocHollowChunks[tallocHollowChunksCount + 1] = next;
                tallocHollowChunksCount += 2;
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
#ifdef TALLOC_TESTING
#include <stdio.h>
TALLOC_DEF void heap_view() {
    if (!tallocMainHeapInfo.initialized)
        talloc_initialize_heap();
    heap_chunk* current = tallocHead;
    printf("tallocChunks: %zu, hollow tallocChunks: %zu\n", chunksCount, tallocHollowChunksCount);
    while (current != 0) {
        if (current->isFree)
            printf("free: %p: %zu\n", current->pointer, current->count);
        else
            printf("commited: %p: %zu\n", current->pointer, current->count);
        current = current->next;
    }
}
#endif // TALLOC_TESTING
#endif // ifndef TALLOC_IMPLEMENTATION
#endif // ifndef TINY_ALLOC_SINGLE_HEADER_H_