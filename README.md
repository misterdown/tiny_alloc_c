# tiny_alloc_c
A very simple memory allocation implementation in pure C.

- [Usage](#usage)
- [Features](#features)
- [Examples](#examples)
- [License](#license)

# Usage
Useless, but funny

# Features
- `talloc` - allocating memory
- `trealloc` - reallocating memory
- `tfree` - freeing allocated memory
- `talloc_heap_view` - printing the heap and chunks info

## talloc
```C
void* talloc(size_t count);
```

A function that allocates an amount of memory equal to `count`. Returns `0` if the function fails for some reason.

If the function returns `0`, it is usually due to the virtual heap size being too small. You can change its size by modifying `TALLOC_MAX_HEAP_SIZE`.

By default, this value is `1024 * 1024 * 4`, which is 4 megabytes (mebibytes).

If this is the first call to this function, it initializes the heap, and an assert is triggered in case of failure.

## trealloc
```C
void* trealloc(void* pointer, size_t count);
```

Reallocates memory for `pointer` with a size of `count`.

If the function returns `0`, it is usually due to the virtual heap size being too small. You can change its size by modifying `TALLOC_MAX_HEAP_SIZE`.

If this is the first call to this function, it initializes the heap, and an assert is triggered in case of failure.

## tfree
```C
void tfree(void* pointer);
```

Deallocates the memory allocated for `pointer`.

If for some reason the function cannot free the memory for this pointer, it does nothing.

## talloc_heap_view
```C
void talloc_heap_view();
```

Prints to stdout basic information about the heap and chunks used for the operation of the `talloc` and `tfree` functions.

If this is the first call to this function, it initializes the heap, and an assert is triggered in case of failure.

# Examples
```C
#define TALLOC_IMPLEMENTATION
#include "tiny_alloc_single_header.h"
#include <stdio.h>
#include <string.h>

int main() {
    char* pointer1 = talloc(128);
    char* pointer2 = talloc(51);
    assert(pointer1 != 0);
    assert(pointer2 != 0);
    strcpy(pointer1, "beleberda bum bim brbrbrbbrbr !!!---");
    strcpy(pointer2, "beleberda bum bim brbrbrbbrbr !!!--- part two! pis");
    printf("%s\n", pointer1);
    printf("%s\n", pointer2);
    tfree(pointer2);
    tfree(pointer1);

    return 0;
}
```

A huge thank you to the creator(s) of [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) for providing the wonderful opportunity to test my wacky ideas.

```C
#define TALLOC_IMPLEMENTATION
#define TALLOC_TESTING
#define TALLOC_MAX_HEAP_SIZE (1024 * 1024 * 8)
#include "tiny_alloc_single_header.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC talloc
#define STBI_FREE tfree
#define STBI_REALLOC trealloc
#include "stb_image.h"

#include <stdio.h>

int main() {
    int x;
    int y;
    int c;
    stbi_uc* data = stbi_load("F:/projects/tiny_malloc/image.jpg", &x, &y, &c, 3);
    assert(data != 0);
    // stbi for SOME REASON allocates (x * y * c) + 1 bytes for jpeg images
    talloc_heap_view();
    stbi_image_free(data);
    printf("%s", "success");
    return 0;
}
```

## License

This library is licensed under the MIT License. See the [LICENSE](LICENSE) file for more details.
