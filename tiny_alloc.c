/*  tiny_alloc.c
    MIT License

    Copyright (c) 2024 Shigapov Aidar

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "tiny_alloc.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#define TALLOC_MAX_HEAP_SIZE (1024*1024*8) // 4 mebibyte
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

static heap_info tallocMainHeapInfo = {};
static heap_chunk tallocChunks[TALLOC_MAX_HEAP_CHUNKS] = {0};
static size_t tallocChunksCount = 0;
static heap_chunk* tallocHollowChunks[TALLOC_MAX_HEAP_CHUNKS] = {0};
static size_t tallocHollowChunksCount = 0;
static heap_chunk* tallocHead;

void initialize_chunks() {
    tallocChunks[0].count = TALLOC_MAX_HEAP_SIZE;
    tallocChunks[0].pointer = (void*)tallocMainHeapInfo.heapPointer;
    tallocChunks[0].isFree = true;
    tallocChunks[0].prev = 0;
    tallocChunks[0].next = 0;
    tallocChunksCount = 1;
    tallocHead = &tallocChunks[0];
    for (size_t i = 1; i < TALLOC_MAX_HEAP_CHUNKS; ++i) {
        tallocHollowChunks[i] = &tallocChunks[i]; 
    }
    tallocHollowChunksCount = TALLOC_MAX_HEAP_CHUNKS - 1;
}
#if TALLOC_USE_STATIC
void talloc_initialize_heap() {
    tallocMainHeapInfo.initialized = true;
    initialize_chunks();
}
#else
#ifdef __linux__
#include <unistd.h>
#include <sys/mman.h>
void talloc_initialize_heap() {
    tallocMainHeapInfo.heapPointer = (char*)mmap(0, TALLOC_MAX_HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    tallocMainHeapInfo.initialized = tallocMainHeapInfo.heapPointer != MAP_FAILED;
    assert(tallocMainHeapInfo.initialized);
    initialize_chunks();
}
#elif (defined __WIN32)
#include <windows.h>
void talloc_initialize_heap() {
    tallocMainHeapInfo.heapPointer = (char*)VirtualAlloc(0, TALLOC_MAX_HEAP_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    tallocMainHeapInfo.initialized = tallocMainHeapInfo.heapPointer != 0;
    assert(tallocMainHeapInfo.initialized);
    initialize_chunks();
}
#else
static_assert(false, "not supported");
#endif // linux or windows
#endif // TALLOC_USE_STATIC

heap_chunk* talloc__pop_get_back_chunk() {
    heap_chunk* backChunk = tallocHollowChunks[tallocHollowChunksCount - 1];
    --tallocHollowChunksCount;
    ++tallocChunksCount;
    return backChunk;
}
void talloc__return_chunk(heap_chunk* toReturn) {
    tallocHollowChunks[tallocHollowChunksCount] = toReturn;
    ++tallocHollowChunksCount;
    --tallocChunksCount;
}
// does not safe. Pass only valid chunk, please
void talloc__free_chunk(heap_chunk* chunk) {
    chunk->isFree = true;
    heap_chunk* prev = chunk->prev;
    heap_chunk* next = chunk->next;
    const int mask = 
        ((next != 0) && (next->isFree) ? 1 : 0) |
        ((prev != 0) && (prev->isFree) ? 2 : 0);
    switch (mask) {
    case 1: {
        chunk->count += next->count;
        chunk->next = next->next;
        if (next->next != 0)
            next->next->prev = chunk;
        talloc__return_chunk(next);
        break;
    } 
    case 2: {
        prev->count += chunk->count;
        prev->next = next;
        if (next != 0)
            next->prev = prev;
        talloc__return_chunk(chunk);
        break;
    } 
    case 3: {
        prev->count += chunk->count + next->count;
        prev->next = next->next;
        if (next->next != 0)
            next->next->prev = prev;
        talloc__return_chunk(chunk);
        talloc__return_chunk(next);
        break;
    }
    default:
        break;
    }
}
// does not safe. Pass only valid FREE chunk, please
void* talloc__alloc_on_chunk(heap_chunk* chunk, size_t count) {
    if (chunk->next == 0) { // is a tale
        heap_chunk* next = chunk->next = talloc__pop_get_back_chunk();
        next->count = count;
        next->prev = chunk;
        next->next = 0;
        next->isFree = false;    
        next->pointer = chunk->pointer + chunk->count - count;
        chunk->count -= count;

        if (chunk->count == 0) {
            if (chunk->prev != 0)
                chunk->prev->next = 0;
            if (chunk == tallocHead) {
                tallocHead = chunk->next;
                tallocHead->prev = 0;
            };
            talloc__return_chunk(chunk);
        }
        return next->pointer;
    } else {
        heap_chunk* newNext = talloc__pop_get_back_chunk();
        heap_chunk* curNext = chunk->next;
        newNext->next = curNext;
        newNext->prev = chunk;
        chunk->next = newNext;
        curNext->prev = newNext;    
        newNext->isFree = false;
        newNext->count = count;
        newNext->pointer = chunk->pointer + chunk->count - count;
        chunk->count -= count;
        if (chunk->count == 0) {
            if (chunk->prev != 0)
                chunk->prev->next = newNext;
            if (chunk == tallocHead) {
                tallocHead = chunk->next;
                tallocHead->prev = 0;
            }
            talloc__return_chunk(chunk);
        }
        return newNext->pointer;
    }
    return 0;
}
void* talloc(size_t count) {
    if ((tallocChunksCount > TALLOC_MAX_HEAP_CHUNKS) || (count == 0))
        return 0;
    if (!tallocMainHeapInfo.initialized)
        talloc_initialize_heap();
    heap_chunk* current = tallocHead;
    while (current != 0) {
        if ((current->isFree) && (current->count >= count))
            return talloc__alloc_on_chunk(current, count);
        current = current->next;
    }
    return 0;
}
void tfree(void* pointer) {
    heap_chunk* current = tallocHead;
    while (current != 0) {
        if (current->pointer == pointer) {
            talloc__free_chunk(current);
            return;
        }
        current = current->next;
    }
}
void* trealloc(void* pointer, size_t count) {
    if ((tallocChunksCount > TALLOC_MAX_HEAP_CHUNKS) || (count == 0)) {
        tfree(pointer);
        return 0;
    }
   
    if (!tallocMainHeapInfo.initialized)
        talloc_initialize_heap();
    heap_chunk* current = tallocHead;
    while (current != 0) {
        if (current->pointer == pointer) {
            if (current->count > count) {
                if ((current->next == 0) || ((current->next != 0) && !current->next->isFree)) {
                    heap_chunk* curNext = current->next;
                    heap_chunk* newNext = current->next = talloc__pop_get_back_chunk();
                    newNext->isFree = true;
                    newNext->count = current->count - count;
                    newNext->next = curNext;
                    if ((curNext != 0) && (curNext->next != 0))
                        curNext->next->prev = newNext;
                    newNext->prev = current;
                } else {
                    heap_chunk* next = current->next;
                    size_t delta = current->count - count;
                    if (delta == next->count) {
                        current->next = next->next;
                        if (next->next != 0)
                            next->next->prev = current;
                        talloc__return_chunk(next);
                    } else {
                        next->pointer -= delta;
                        next->count += delta;
                    }
                }
                current->count = count;
                return current->pointer;
            } else if (current->count < count) {
                heap_chunk* next = current->next;
                if (next == 0) {
                    talloc__free_chunk(current);
                    return talloc(count);
                } else if (next->isFree) {
                    size_t delta = count - current->count;
                    if (next->count < delta) {
                        talloc__free_chunk(current);
                        return talloc(count);
                    } else if (next->count > delta){ 
                        current->count += delta;
                        next->count -= delta;
                        next->pointer += delta;
                        return current->pointer;
                    } else { //  equal
                        current->count += delta;
                        current->next = next->next;
                        if (next->next != 0)
                            next->next->prev = current;
                        talloc__return_chunk(next);
                        return current->pointer;
                    }
                } else {
                    talloc__free_chunk(current);
                    return talloc(count);
                }
            } else { // equal
                return current->pointer;
            }
            break;
        }
        current = current->next;
    }
    return talloc(count);
}

void talloc_heap_view() {
    if (!tallocMainHeapInfo.initialized)
        talloc_initialize_heap();
    heap_chunk* current = tallocHead;
    printf("tallocChunks: %zu, hollow tallocChunks: %zu\n", tallocChunksCount, tallocHollowChunksCount);
    while (current != 0) {
        if (current->isFree)
            printf("free: %p: %zu\n", current->pointer, current->count);
        else
            printf("commited: %p: %zu\n", current->pointer, current->count);
        current = current->next;
    }
}