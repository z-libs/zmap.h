# zmap.h

`zmap.h` is a high-performance, type-safe hash map library for C11 and C++.

It implements **Open Addressing** with **Robin Hood Hashing** and **Fibonacci Indexing**, achieving exceptional cache locality and resilience against high load factors (up to 0.9).

Unlike standard C hash maps (like `uthash` or chaining implementations) that suffer from pointer chasing and malloc overhead, `zmap.h` stores data in contiguous memory. It uses C11 `_Generic` selection to generate fully typed implementations, providing the performance of a handwritten map with the ease of use of a generic library.

## Features

* **Robin Hood Hashing**: Minimizes probe variance, enabling high load factors without performance degradation.
* **Fibonacci Indexing**: Mathematically optimal distribution for all key types, protecting against bad hash functions.
* **Type Safety**: Compiler errors if you mix types. No `void*` casting.
* **Stable Maps**: Optional "Stable" mode where pointers to values remain valid even after resizes.
* **WyHash Support**: Automatically uses the ultra-fast WyHash algorithm if `zhash.h` is present.
* **C++ Support**: Includes a zero-cost C++ wrapper (`z_map::map`) with STL-compatible iterators.
* **Header Only**: No linking required. Drop it in and go.

## Benchmarks

This repository automatically runs benchmarks on every commit via GitHub Actions.

**[View Live Benchmark Results.](benchmark_results.txt)**

### Reference Performance (Nitro AN515-55)
Comparison vs **uthash** (standard C hash map) managing 1,000,000 string keys:

| Operation | zmap (WyHash) | uthash (Jenkin's) | Speedup |
| :--- | :--- | :--- | :--- |
| **Insert** | 0.1637s | 0.3269s | **2.00x Faster** |
| **Lookup (Hit)** | 0.1124s | 0.2582s | **2.30x Faster** |
| **Lookup (Miss)** | 0.1092s | 0.3449s | **3.16x Faster** |
| **Delete All** | 0.0024s | 0.1062s | **44.29x Faster** |

![Benchmark](https://raw.githubusercontent.com/z-libs/zmap.h/8cdc3780a137df137c1e1b3eaff088b1c5f94d53/svg/benchmark.svg)

> **Note on CI Performance:** The live results in `benchmark_results.txt` run on shared cloud runners (Ubuntu/Azure). The raw timings might be slower than a local native machine due to virtualization noise.

## Installation

`zmap.h` works best when you use the provided scanner script to manage type registrations, though it can be used manually.

1.  Copy `zmap.h` (and `zcommon.h` if separated) to your project's include folder.
2. **(Recommended)** Copy `zhash.h` to the same folder (improved string hashing performance).
3.  Add the `z-core` tools (optional but recommended).
```bash
git submodule add [https://github.com/z-libs/z-core.git](https://github.com/z-libs/z-core.git) z-core
```

## Usage: C

For C projects, you define the map types you need using a macro that the scanner detects.

```c
#include <stdio.h>
#include <string.h>
#include "zmap.h"

// Helper functions are required for hashing and comparing keys.
// (You can write your own or use the provided macros).
uint32_t hash_str(char *key)   { return ZMAP_HASH_STR(key); }
int      cmp_str(char *a, char *b) { return strcmp(a, b); }

// Request the map types you need.
// Syntax: DEFINE_MAP_TYPE(KeyType, ValueType, ShortName).
DEFINE_MAP_TYPE(char*, int, StrInt)

int main(void)
{
    // Initialize (Standard C style).
    map_StrInt scores = map_init(StrInt, hash_str, cmp_str);

    map_put(&scores, "Alice", 100);
    map_put(&scores, "Bob", 200);

    // Get returns a pointer to the value, or NULL.
    int *score = map_get(&scores, "Alice");
    if (score)
    {
        printf("Alice: %d\n", *score);
    }
    
    // Cleanup.
    map_free(&scores);
    return 0;
}
```

## Usage: C++

The library detects C++ compilers automatically. The C++ wrapper lives in the **`z_map`** namespace.

```cpp
#include <iostream>
#include <string>
#include "zmap.h"

// Request types (scanner sees this even in .cpp files).
DEFINE_MAP_TYPE(int, float, IntFloat)

// Helpers
uint32_t hash_int(int k) { return ZMAP_HASH_SCALAR(k); }
int cmp_int(int a, int b) { return a - b; }

int main()
{
    // RAII handles memory automatically.
    // Constructor takes hash and compare functions.
    z_map::map<int, float> data(hash_int, cmp_int);

    // STL-like API.
    data.put(10, 1.5f);
    data.insert_or_assign(20, 2.5f);

    // Check existence.
    if (data.contains(10)) {
        std::cout << "Found 10: " << *data.get(10) << "\n";
    }

    // Range-based for loops.
    for(auto& item : data) {
        std::cout << "Key: " << item.key << ", Val: " << item.value << "\n";
    }

    return 0;
}
```

## Compilation Guide

Since `zmap.h` relies on code generation for its type safety, here you have the guide if you use the scanner.

**[Read the Compilation Guide](examples/README.md)** for detailed instructions on how to use `zscanner.py`.

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

// **IT HAS TO BE INCLUDED AFTER, NOT BEFORE**.
#include "zmap.h"

#endif
```

* Include `"my_maps.h"` instead of `"zmap.h"` in your C files.

## Advanced Features

### Stable Maps (Pointer Stability)

Standard `zmap` stores values *inline* in the bucket array for speed. However, if the map resizes, these values move in memory, invalidating any pointers you might be holding.

If you need pointers to remain valid (for example, for linking structures), use a **Stable Map**.

```c
// Define a stable map.
DEFINE_STABLE_MAP_TYPE(char*, MyStruct, StableStr)

// Initialize.
map_StableStr m = map_init_stable(StableStr, hash_fn, cmp_fn);

// Pointers returned by map_get are now stable forever (until removal).
MyStruct *ptr = map_get(&m, "key");
```

### High-Performance Hashing (`zhash.h`)

`zmap.h` automatically detects if `zhash.h` is available.
* **Without `zhash.h`:** Uses FNV-1a (Simple, fast for ints, slower for long strings).
* **With `zhash.h`:** Uses **WyHash** (SIMD-optimized, 8 bytes/cycle).

**Recommendation:** Always include `zhash.h` if you use string keys.

### Identity Hashing (Bitcoin / UUIDs)

If your keys are already random (for example, SHA-256 hashes, UUIDs, pointers), **do not hash them again**. It wastes CPU. Use an "Identity Hash" that just casts the first 4 bytes of your key to `uint32_t`.

```c
uint32_t hash_identity(uint256 k)
{
    uint32_t h; memcpy(&h, &k, 4); return h;
}
```

Because `zmap` uses **Fibonacci Indexing**, it will take these raw bits and distribute them perfectly across the map, giving you $O(1)$ performance with near-zero hashing cost.

## API Reference (C)

`zmap.h` uses C11 `_Generic` dispatch to provide a unified, polymorphic API. You generally use the same macros (`map_put`, `map_get`) regardless of whether you are using a Standard or Stable map.

**Initialization & Lifecycle**

| Macro | Description | Standard vs. Stable Behavior |
| :--- | :--- | :--- |
| `map_init(Name, h, c)` | Creates a **Standard** map with default load factor (0.9). | Stores values **inline** for maximum cache locality. |
| `map_init_stable(Name, h, c)` | Creates a **Stable** map with default load factor. | Stores values **on the heap**. Pointers to values remain valid after resizes. |
| `map_init_ext_##Name(...)` | *Advanced.* Initialize with a custom load factor (0.1 - 0.95). | Same as above. |
| `map_free(m)` | Frees all memory associated with the map. | **Standard:** Frees the bucket array.<br>**Stable:** Frees every allocated value individually, then the array. |
| `map_clear(m)` | Removes all items but keeps the allocated capacity. | **Stable:** Frees all value pointers but keeps the bucket array. |
| `map_set_seed(m, s)` | Sets the random seed for the hash function. | Useful for protecting against HashDoS attacks. |

**Iteration**

Iteration is uniform across both types. The iterator skips empty and deleted slots.

| Macro | Description |
| :--- | :--- |
| `map_iter_init(Name, m)` | Creates a new iterator struct (`zmap_iter_Name`) starting at index 0. |
| `map_iter_next(it, k, v)` | Advances iterator.<br>**Out:** Writes key to `*k` and value to `*v`.<br>**Return:** `true` if found, `false` if end of map. |

**Example:**
```c
// Works identically for map_StrInt (Standard) or map_StableStr (Stable)
zmap_iter_StrInt it = map_iter_init(StrInt, &my_map);
char *key; int val;

while (map_iter_next(&it, &key, &val)) {
    printf("Key: %s, Val: %d\n", key, val);
}
```

**Helper Macros**

| Function/Macro | Description |
| :--- | :--- |
| `ZMAP_HASH_STR(char *s, seed)` | Optimized hash for C-strings (uses WyHash if `zhash.h` is present). |
| `ZMAP_HASH_SCALAR(val, seed)` | Optimized hash for integers/pointers/floats (uses FNV-1a fallback or WyHash). |

### Extensions (Experimental)

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

## API Reference (C++)

The C++ wrapper lives in the **`z_map`** namespace. It strictly adheres to RAII principles and delegates all logic to the underlying C implementation.

### `class z_map::map<K, V>`

**Constructors & Management**

| Method | Description |
| :--- | :--- |
| `map(hash_fn, cmp_fn)` | Constructs map with specific helpers. |
| `~map()` | Destructor. Automatically calls `map_free`. |
| `operator=` | Move assignment operator. |
| `size()` | Returns current number of elements. |
| `empty()` | Returns `true` if size is 0. |
| `clear()` | Clears items but keeps capacity. |

**Access & Modification**

| Method | Description |
| :--- | :--- |
| `put(k, v)` | Inserts or updates key-value pair. Throws `std::bad_alloc` on failure. |
| `insert_or_assign(k, v)` | Alias for `put`. |
| `get(k)` | Returns `V*` or `const V*`. Returns `nullptr` if not found. |
| `contains(k)` | Returns `true` if key exists. |
| `erase(k)` | Removes the key if present. |

**Iterators**

| Method | Description |
| :--- | :--- |
| `begin()`, `end()` | Forward iterators compatible with STL. Returns reference to bucket (`{key, value}`). |
| `cbegin()`, `cend()` | Const iterators for read-only access. |

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
