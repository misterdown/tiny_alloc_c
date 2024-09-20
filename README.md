# tiny_alloc_c
Very simple memory malloc implementation in pure C.

- [Usage](#usage)
- [Features](#features)
- [Examples](#examples)
- [License](#license)

# Usage
Useless, but funny

# Features
 - `talloc` - allocating memory
 - `free` - free allocating memory
 - `heap_view` - print the heap and chunks info
## talloc
```C
 void* talloc(size_t count);
```

A function that allocates an amount of memory equal to count. Returns 0 if the function fails for some reason.

If the function returns 0, it is USUALLY due to the virtual heap size being too small. You can change its size by modifying TALLOC_MAX_HEAP_SIZE.

By default, this value is 102410244, which is 4 megabytes (mebibytes).

If this is the first call to this function, it initializes the heap, and an assert is triggered in case of failure.

## tfree
```C
void tfree(void* pointer);
```

Deallocates the memory allocated for `pointer` `talloc`.

If for some reason the function cannot free the memory for this pointer, it does nothing.

## heap_view
```C
void heap_view();
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
#define TALLOC_MAX_HEAP_SIZE (1024*1024*16) 
#include "tiny_alloc_single_header.h"

void* trealloc(void* pointer, size_t count) { // not implement yet
    tfree(pointer);
    return talloc(count);
}

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
    stbi_uc* data = stbi_load("F:/projects/tiny_malloc/image.jpg", &x,& y, &c, 3);
    assert(data != 0);
    stbi_image_free(data);
    printf("%s", "success");
    return 0;
}
```

## License

This library is licensed under the MIT License. See the [LICENSE](LICENSE) file for more details.
