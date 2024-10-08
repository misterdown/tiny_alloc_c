/*  tiny_alloc.h
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
#ifndef TINY_ALLOC_H_
#include <stdint.h>
/** 
 * @brief   A function that allocates an amount of memory equal to `count`. Returns `0` if the function fails for some reason.
 *          If the function returns `0`, it is usually due to the virtual heap size being too small. You can change its size by modifying `TALLOC_MAX_HEAP_SIZE`.
 *          By default, this value is `1024 * 1024 * 4`, which is 4 megabytes (mebibytes).
 *          If this is the first call to this function, it initializes the heap, and an assert is triggered in case of failure.
 * @param count count of bytes to allocate.
 * @return Valid or zero pointer.
 */
 void* talloc(size_t count);

/** 
 * @brief   Reallocates memory for `pointer` with a size of `count`.
 *          If the function returns `0`, it is usually due to the virtual heap size being too small. You can change its size by modifying `TALLOC_MAX_HEAP_SIZE`.
 *          If this is the first call to this function, it initializes the heap, and an assert is triggered in case of failure.
 *          Copy old data to new pointer.
 * @param pointer pointer to reallocate.
 * @param count count of bytes to allocate.
 * @param copyOld copy and clear old data or not.
 * @return Valid or zero pointer.
 */ 
void* trealloc(void* pointer, size_t count, const int copyOld);

/// @brief Deallocates the memory allocated for `pointer`. If for some reason the function cannot free the memory for this pointer, it does nothing. @param pointer pointer to free.
void tfree(void* pointer);

/// @brief Prints to stdout basic information about the heap and chunks used for the operation of the `talloc` and `tfree` functions. If this is the first call to this function, it initializes the heap, and an assert is triggered in case of failure.
void talloc_heap_view();
#endif