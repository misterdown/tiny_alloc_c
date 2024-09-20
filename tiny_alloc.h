#ifndef TINY_ALLOC_H_
#include <stdint.h>
/// @brief A function that allocates an amount of memory equal to count. Returns 0 if the function fails for some reason.
///        If this is the first call to this function, it initializes the heap, and an assert is triggered in case of failure.
/// @param count count of bytes to allocate.
/// @return Valid or zero pointer.
void* talloc(size_t count);
/// @brief Deallocates the memory allocated for `pointer` `talloc`. If for some reason the function cannot free the memory for this pointer, it does nothing. @param pointer pointer to free.
void tfree(void* pointer);
/// @brief Prints to stdout basic information about the heap and chunks used for the operation of the `talloc` and `tfree` functions. If this is the first call to this function, it initializes the heap, and an assert is triggered in case of failure.
void heap_view();
#endif