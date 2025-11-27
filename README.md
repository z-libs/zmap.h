# zmap.h

`zmap.h` provides generic hash maps (dictionaries) for C projects. It uses open addressing with linear probing for high performance and cache efficiency. Unlike typical C map implementations that force `void*` casting or string-only keys, `zmap.h` uses C11 `_Generic` selection to generate fully typed, type-safe implementations for your specific key-value pairs.

## Features

* **Type Safety**: Compiler errors if you try to put a `float` key into an `int` map.
* **Native Performance**: Open addressing with linear probing reduces pointer chasing and malloc overhead compared to chained hashmaps.
* **Zero Boilerplate**: Use the **Z-Scanner** tool to automatically generate type registrations.
* **Header Only**: No linking required.
* **Memory Agnostic**: Supports custom allocators (Arenas, Pools, Debuggers).
* **Zero Dependencies**: Only standard C headers used.

## Quick Start (Automated)

The easiest way to use `zmap.h` is with the **Z-Scanner** tool, which scans your code and handles the boilerplate for you.

### 1. Setup

Add `zmap.h` and the `z-core` tools to your project:

```bash
# Copy zmap.h to your root or include folder.
git submodule add https://github.com/z-libs/z-core.git z-core
```

### 2. Write Code

You don't need a separate registry file. Just define the types you need right where you use them (or in your own headers).

```c
#include <stdio.h>
#include <string.h>
#include "zmap.h"

// Define a custom struct if needed.
typedef struct { int id; } Product;

// Helper functions are required for hashing and comparing keys.
uint32_t hash_str(char *key)   { return ZMAP_HASH_STR(key); }
int      cmp_str(char *a, char *b) { return strcmp(a, b); }

// Request the map types you need.
// Syntax: DEFINE_MAP_TYPE(KeyType, ValueType, ShortName).
DEFINE_MAP_TYPE(char*, int, StrInt)
DEFINE_MAP_TYPE(int, float, IntFloat)

int main(void)
{
    // Initialize with helper functions.
    map_StrInt scores = map_init(StrInt, hash_str, cmp_str);

    map_put(&scores, "Alice", 100);
    map_put(&scores, "Bob", 200);

    // Get returns a pointer to the value, or NULL.
    int *score = map_get(&scores, "Alice");
    if (score)
    {
        printf("Alice: %d\n", *score);
    }
    
    map_free(&scores);
    return 0;
}
```

### 3. Build

Run the scanner before compiling. It will create a header that `zmap.h` automatically detects.

```bash
# Scan your source folder (for example, src/ or .) and output to 'z_registry.h'.
python3 z-core/zscanner.py . z_registry.h

# Compile (Include the folder where z_registry.h lives).
gcc main.c -I. -o game
```

## Manual Setup

If you cannot use Python or prefer manual control, you can use the **Registry Header** approach.

* Create a file named `my_maps.h`.
* Register your types using X-Macros.

```c
#ifndef MY_MAPS_H
#define MY_MAPS_H

// Register Types.
// Syntax: X(KeyType, ValueType, ShortName).
#define REGISTER_MAP_TYPES(X) \
    X(char*, int, StrInt)     \
    X(int, float, IntFloat)

// Include Library (AFTER defining the macro).
#include "zmap.h"

#endif
```

* Include `"my_maps.h"` instead of `"zmap.h"` in your C files.

## API Reference

`zmap.h` uses C11 `_Generic` to automatically select the correct function implementation based on the map pointer you pass.

### Initialization & Management

| Macro | Description |
| :--- | :--- |
| `map_init(Name, hash_fn, cmp_fn)` | Returns an empty map struct. Requires function pointers for hashing and comparing keys. |
| `map_free(m)` | Frees the internal bucket array and zeroes the map structure. |
| `map_clear(m)` | Clears all items (memset to 0) but keeps the allocated memory capacity. |
| `map_size(m)` | Returns the number of active items (`size_t`). |

### Data Access

| Macro | Description |
| :--- | :--- |
| `map_put(m, key, val)` | Inserts the key-value pair. If the key exists, updates the value. Returns `Z_OK` or `Z_ERR`. |
| `map_get(m, key)` | Returns a **pointer** to the value associated with `key`, or `NULL` if not found. |
| `map_remove(m, key)` | Removes the item associated with `key`. Does nothing if the key is missing. |

### Helper Functions

The library provides built-in hashing helpers you can wrap in your own functions:

| Function/Macro | Description |
| :--- | :--- |
| `ZMAP_HASH_STR(char *s)` | FNV-1a hash helper for null-terminated C strings. |
| `ZMAP_HASH_SCALAR(val)` | FNV-1a hash helper for scalar types (`int`, `long`, `float`, etc.). |

## Extensions (Experimental)

If you are using a compiler that supports `__attribute__((cleanup))` (like GCC or Clang), you can use the **Auto-Cleanup** extension to automatically free maps when they go out of scope.

| Macro | Description |
| :--- | :--- |
| `map_autofree(Name)` | Declares a map that automatically calls `map_free` when the variable leaves scope (RAII style). |

**Example:**

```c
void process_data()
{
    // 'cache' will be automatically freed when this function returns.
    map_autofree(StrInt) cache = map_init(StrInt, hash_str, cmp_str);
    map_put(&cache, "Key", 100);
}
```

> **Disable Extensions:** To force standard compliance and disable these extensions, define `Z_NO_EXTENSIONS` before including the library.

## Memory Management

By default, `zmap.h` uses the standard C library functions (`malloc`, `calloc`, `free`).

However, you can override these to use your own memory subsystem (e.g., **Memory Arenas**, **Pools**, or **Debug Allocators**).

### First Option: Global Override (Recommended)

To use a custom allocator, define the `Z_` macros **inside your registry header**, immediately before including `zmap.h`.

```c
#ifndef MY_MAPS_H
#define MY_MAPS_H

// Define your custom memory macros **HERE**.
#include "my_memory_system.h"

// IMPORTANT: Override all four to prevent mixing allocators.
// (Maps use calloc for buckets to detect empty slots).
#define Z_MALLOC(sz)      my_custom_alloc(sz)
#define Z_CALLOC(n, sz)   my_custom_calloc(n, sz)
#define Z_REALLOC(n, sz)  my_custom_realloc(p, sz)
#define Z_FREE(p)         my_custom_free(p)

// Then include the library.
#include "zmap.h"

// ... Register types ...

#endif
```

> **Note:** You **must** override **all four macros** (`MALLOC`, `CALLOC`, `REALLOC`, `FREE`) if you override one, to ensure consistency with all libraries.

### Second Option: Library-Specific Override (Advanced)

If you need different allocators for different containers (e.g., an Arena for Maps but the Heap for Vectors), you can use the library-specific macros. These take priority over the global `Z_` macros.

```c
// Example: Maps use a Dict Arena, everything else uses standard malloc.
#define Z_MAP_CALLOC(n, sz)  arena_alloc_zero(dict_arena, (n) * (sz))
#define Z_MAP_FREE(p)        /* no-op for linear arena */

#include "zmap.h"
#include "zvec.h" // zvec will still use standard malloc!
```

## Notes

### Why do I need to provide a "Short Name"?

In `DEFINE_MAP_TYPE(Key, Val, ShortName)`, you must provide a Short Name.

```c
//              Key    Val   ShortName
DEFINE_MAP_TYPE(char*, int,  StrInt)
```

The reason is that C macros cannot handle special characters (like `*` or space) when generating names. The library tries to create structs and functions by gluing `map_` + `Name`.

If you tried to generate `map_char*`, it would be a syntax error. By passing `StrInt`, it correctly generates `map_StrInt`, `map_put_StrInt`, etc.
