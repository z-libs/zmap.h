
#ifndef ZMAP_H
#define ZMAP_H

#include "zcommon.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Configuration & Allocators */

#ifndef Z_MAP_MALLOC
    #define Z_MAP_MALLOC(sz)      Z_MALLOC(sz)
#endif

#ifndef Z_MAP_CALLOC
    #define Z_MAP_CALLOC(n, sz)   Z_CALLOC(n, sz)
#endif

#ifndef Z_MAP_REALLOC
    #define Z_MAP_REALLOC(p, sz)  Z_REALLOC(p, sz)
#endif

#ifndef Z_MAP_FREE
    #define Z_MAP_FREE(p)         Z_FREE(p)
#endif

// Default load factor (0.75 is standard for open addressing).
// Can be tuned per-map using map_init_chk.
#define ZMAP_DEFAULT_LOAD 0.75f


/* Hashing Helper */

// FNV-1a Hash Implementation (32-bit).
static inline uint32_t zmap_default_hash(const void *key, size_t len)
{
    uint32_t hash = 2166136261u;
    const uint8_t *data = (const uint8_t *)key;
    for (size_t i = 0; i < len; i++)
    {
        hash ^= data[i];
        hash *= 16777619;
    }
    return hash;
}

// Helper macros for hashing scalars and C-strings.
#define ZMAP_HASH_SCALAR(k) zmap_default_hash(&(k), sizeof(k))
#define ZMAP_HASH_STR(k)    zmap_default_hash((k), strlen(k))


/* Internal Data Structures */

typedef enum 
{
    ZMAP_EMPTY = 0,
    ZMAP_OCCUPIED,
    ZMAP_DELETED
} zmap_state;


// Our blueprint.
#define DEFINE_MAP_TYPE(KeyT, ValT, Name)                                                                       \
                                                                                                                \
/* The bucket struct holding individual entries. */                                                             \
typedef struct {                                                                                                \
    KeyT key;                                                                                                   \
    ValT value;                                                                                                 \
    zmap_state state;                                                                                           \
} zmap_bucket_##Name;                                                                                           \
                                                                                                                \
/* The main map struct. */                                                                                      \
typedef struct {                                                                                                \
    zmap_bucket_##Name *buckets;                                                                                \
    size_t capacity;                                                                                            \
    size_t count;       /* Number of active items */                                                            \
    size_t occupied;    /* Active + Deleted items (for load factor) */                                          \
    size_t threshold;   /* Grow when occupied >= threshold */                                                   \
    float  load_factor;                                                                                         \
    uint32_t (*hash_func)(KeyT);                                                                                \
    int      (*cmp_func)(KeyT, KeyT);                                                                           \
} map_##Name;                                                                                                   \
                                                                                                                \
/* Iterator state struct. */                                                                                    \
typedef struct {                                                                                                \
    map_##Name *map;                                                                                            \
    size_t index;                                                                                               \
} zmap_iter_##Name;                                                                                             \
                                                                                                                \
/* Frees the internal bucket memory. Does NOT free the map pointer itself. */                                   \
static inline void map_free_##Name(map_##Name *m) {                                                             \
    Z_MAP_FREE(m->buckets);                                                                                     \
    *m = (map_##Name){0};                                                                                       \
}                                                                                                               \
                                                                                                                \
/* Initializes a map with a custom load factor. */                                                              \
static inline map_##Name map_init_ext_##Name(uint32_t (*h)(KeyT), int (*c)(KeyT, KeyT), float load) {           \
    return (map_##Name){                                                                                        \
        .hash_func = h,                                                                                         \
        .cmp_func = c,                                                                                          \
        .load_factor = (load <= 0.1f || load > 0.95f) ? ZMAP_DEFAULT_LOAD : load,                               \
        .threshold = 0                                                                                          \
    };                                                                                                          \
}                                                                                                               \
                                                                                                                \
/* Initializes a map with default settings (Load Factor 0.75). */                                               \
static inline map_##Name map_init_##Name(uint32_t (*h)(KeyT), int (*c)(KeyT, KeyT)) {                           \
    return map_init_ext_##Name(h, c, ZMAP_DEFAULT_LOAD);                                                        \
}                                                                                                               \
                                                                                                                \
/* Internal: Resizes the bucket array and rehashes all active entries. */                                       \
static inline int map_resize_##Name(map_##Name *m, size_t new_cap) {                                            \
    zmap_bucket_##Name *new_buckets = Z_MAP_CALLOC(new_cap, sizeof(zmap_bucket_##Name));                        \
    if (!new_buckets) return Z_ERR;                                                                             \
                                                                                                                \
    for (size_t i = 0; i < m->capacity; i++) {                                                                  \
        if (m->buckets[i].state == ZMAP_OCCUPIED) {                                                             \
            uint32_t hash = m->hash_func(m->buckets[i].key);                                                    \
            size_t idx = hash % new_cap;                                                                        \
            while (new_buckets[idx].state == ZMAP_OCCUPIED) {                                                   \
                idx = (idx + 1) % new_cap;                                                                      \
            }                                                                                                   \
            new_buckets[idx] = m->buckets[i];                                                                   \
        }                                                                                                       \
    }                                                                                                           \
    Z_MAP_FREE(m->buckets);                                                                                     \
    m->buckets = new_buckets;                                                                                   \
    m->capacity = new_cap;                                                                                      \
    m->occupied = m->count; /* Deleted items are purged during resize */                                        \
    m->threshold = (size_t)(new_cap * m->load_factor);                                                          \
    return Z_OK;                                                                                                \
}                                                                                                               \
                                                                                                                \
/* Inserts or updates a value. Returns Z_OK on success, Z_ERR on allocation failure. */                         \
static inline int map_put_##Name(map_##Name *m, KeyT key, ValT val) {                                           \
    if (m->occupied >= m->threshold) {                                                                          \
        size_t new_cap = m->capacity == 0 ? 16 : m->capacity * 2;                                               \
        if (map_resize_##Name(m, new_cap) != Z_OK) return Z_ERR;                                                \
    }                                                                                                           \
    uint32_t hash = m->hash_func(key);                                                                          \
    size_t idx = hash % m->capacity;                                                                            \
    size_t deleted_idx = SIZE_MAX;                                                                              \
                                                                                                                \
    for (size_t i = 0; i < m->capacity; i++) {                                                                  \
        zmap_state s = m->buckets[idx].state;                                                                   \
        if (s == ZMAP_EMPTY) {                                                                                  \
            if (deleted_idx != SIZE_MAX) idx = deleted_idx;                                                     \
            else m->occupied++;                                                                                 \
            m->buckets[idx] = (zmap_bucket_##Name){ .key = key, .value = val, .state = ZMAP_OCCUPIED };         \
            m->count++;                                                                                         \
            return Z_OK;                                                                                        \
        }                                                                                                       \
        if (s == ZMAP_DELETED) {                                                                                \
            if (deleted_idx == SIZE_MAX) deleted_idx = idx;                                                     \
        }                                                                                                       \
        else if (m->cmp_func(m->buckets[idx].key, key) == 0) {                                                  \
            m->buckets[idx].value = val;                                                                        \
            return Z_OK;                                                                                        \
        }                                                                                                       \
        idx = (idx + 1) % m->capacity;                                                                          \
    }                                                                                                           \
    return Z_ERR;                                                                                               \
}                                                                                                               \
                                                                                                                \
/* Returns a pointer to the value, or NULL if not found. */                                                     \
static inline ValT* map_get_##Name(map_##Name *m, KeyT key) {                                                   \
    if (m->count == 0) return NULL;                                                                             \
    uint32_t hash = m->hash_func(key);                                                                          \
    size_t idx = hash % m->capacity;                                                                            \
                                                                                                                \
    for (size_t i = 0; i < m->capacity; i++) {                                                                  \
        zmap_state s = m->buckets[idx].state;                                                                   \
        if (s == ZMAP_EMPTY) return NULL;                                                                       \
        if (s == ZMAP_OCCUPIED && m->cmp_func(m->buckets[idx].key, key) == 0) {                                 \
            return &m->buckets[idx].value;                                                                      \
        }                                                                                                       \
        idx = (idx + 1) % m->capacity;                                                                          \
    }                                                                                                           \
    return NULL;                                                                                                \
}                                                                                                               \
                                                                                                                \
/* Returns true if the key exists in the map. */                                                                \
static inline bool map_contains_##Name(map_##Name *m, KeyT key) {                                               \
    return map_get_##Name(m, key) != NULL;                                                                      \
}                                                                                                               \
                                                                                                                \
/* Removes an entry by marking it as DELETED (lazy deletion). */                                                \
static inline void map_remove_##Name(map_##Name *m, KeyT key) {                                                 \
    if (m->count == 0) return;                                                                                  \
    uint32_t hash = m->hash_func(key);                                                                          \
    size_t idx = hash % m->capacity;                                                                            \
                                                                                                                \
    for (size_t i = 0; i < m->capacity; i++) {                                                                  \
        zmap_state s = m->buckets[idx].state;                                                                   \
        if (s == ZMAP_EMPTY) return;                                                                            \
        if (s == ZMAP_OCCUPIED && m->cmp_func(m->buckets[idx].key, key) == 0) {                                 \
            m->buckets[idx].state = ZMAP_DELETED;                                                               \
            m->count--;                                                                                         \
            return;                                                                                             \
        }                                                                                                       \
        idx = (idx + 1) % m->capacity;                                                                          \
    }                                                                                                           \
}                                                                                                               \
                                                                                                                \
/* Creates a new iterator for the map. */                                                                       \
static inline zmap_iter_##Name map_iter_init_##Name(map_##Name *m) {                                            \
    return (zmap_iter_##Name){ .map = m, .index = 0 };                                                          \
}                                                                                                               \
                                                                                                                \
/* Advances the iterator. Returns true and writes key/val if a next item exists. */                             \
static inline bool map_iter_next_##Name(zmap_iter_##Name *it, KeyT *out_k, ValT *out_v) {                       \
    if (!it->map || !it->map->buckets) return false;                                                            \
    while (it->index < it->map->capacity) {                                                                     \
        size_t i = it->index++;                                                                                 \
        if (it->map->buckets[i].state == ZMAP_OCCUPIED) {                                                       \
            if (out_k) *out_k = it->map->buckets[i].key;                                                        \
            if (out_v) *out_v = it->map->buckets[i].value;                                                      \
            return true;                                                                                        \
        }                                                                                                       \
    }                                                                                                           \
    return false;                                                                                               \
}                                                                                                               \
                                                                                                                \
static inline size_t map_size_##Name(map_##Name *m) { return m->count; }                                        \
                                                                                                                \
/* Clears all items (sets count to 0) but keeps memory allocated. */                                            \
static inline void map_clear_##Name(map_##Name *m) {                                                            \
    if (m->capacity > 0) {                                                                                      \
            memset(m->buckets, 0, m->capacity * sizeof(zmap_bucket_##Name));                                    \
    }                                                                                                           \
    m->count = 0;                                                                                               \
    m->occupied = 0;                                                                                            \
}

// X-Macro Entry Points.
// These macros map generic calls (e.g., map_put) to specific implementations
// (for example, map_put_int) based on the type registration.
#define M_PUT_ENTRY(K, V, N)     map_##N*: map_put_##N,
#define M_GET_ENTRY(K, V, N)     map_##N*: map_get_##N,
#define M_HAS_ENTRY(K, V, N)     map_##N*: map_contains_##Name,
#define M_REM_ENTRY(K, V, N)     map_##N*: map_remove_##N,
#define M_FREE_ENTRY(K, V, N)    map_##N*: map_free_##N,
#define M_SIZE_ENTRY(K, V, N)    map_##N*: map_size_##N,
#define M_CLEAR_ENTRY(K, V, N)   map_##N*: map_clear_##N,
#define M_ITER_NEXT_ENTRY(K,V,N) zmap_iter_##N*: map_iter_next_##Name,


/* Generic Accessors */

// Standard Init (Load Factor = 0.75).
#define map_init(Name, h_func, c_func)    map_init_##Name(h_func, c_func)

// Advanced Init (Custom Load Factor).
#define map_init_chk(Name, h, c, load)    map_init_ext_##Name(h, c, load)

// Auto-Cleanup Extension (GCC/Clang).
#if defined(Z_HAS_CLEANUP) && Z_HAS_CLEANUP
    #define map_autofree(Name)  Z_CLEANUP(map_free_##Name) map_##Name
#endif

// Generic API Methods.
#define map_put(m, k, v)      _Generic((m), REGISTER_MAP_TYPES(M_PUT_ENTRY)   default: 0) (m, k, v)
#define map_get(m, k)         _Generic((m), REGISTER_MAP_TYPES(M_GET_ENTRY)   default: (void*)0) (m, k)
#define map_contains(m, k)    _Generic((m), REGISTER_MAP_TYPES(M_HAS_ENTRY)   default: false) (m, k)
#define map_remove(m, k)      _Generic((m), REGISTER_MAP_TYPES(M_REM_ENTRY)   default: (void)0) (m, k)
#define map_free(m)           _Generic((m), REGISTER_MAP_TYPES(M_FREE_ENTRY)  default: (void)0) (m)
#define map_size(m)           _Generic((m), REGISTER_MAP_TYPES(M_SIZE_ENTRY)  default: 0) (m)
#define map_clear(m)          _Generic((m), REGISTER_MAP_TYPES(M_CLEAR_ENTRY) default: (void)0) (m)

/* Iteration Macros */

// usage: zmap_iter_Name it = map_iter_init(Name, &my_map);
#define map_iter_init(Name, m) map_iter_init_##Name(m)
// usage: while (map_iter_next(&it, &key, &val)) { ... }
#define map_iter_next(it, k, v) _Generic((it), REGISTER_MAP_TYPES(M_ITER_NEXT_ENTRY) default: false) (it, k, v)

#endif
