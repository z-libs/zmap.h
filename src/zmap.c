
#ifndef ZMAP_H
#define ZMAP_H

#include "zcommon.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef Z_MAP_MALLOC
    #define Z_MAP_MALLOC(sz)      Z_MALLOC(sz)
#endif
#ifndef Z_MAP_CALLOC
    #define Z_MAP_CALLOC(n, sz)   Z_CALLOC(n, sz)
#endif
#ifndef Z_MAP_FREE
    #define Z_MAP_FREE(p)         Z_FREE(p)
#endif

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

typedef enum 
{
    ZMAP_EMPTY = 0,
    ZMAP_OCCUPIED,
    ZMAP_DELETED
} zmap_state;

#define ZMAP_HASH_SCALAR(k) zmap_default_hash(&(k), sizeof(k))
#define ZMAP_HASH_STR(k)    zmap_default_hash((k), strlen(k))

#define DEFINE_MAP_TYPE(KeyT, ValT, Name)                                                                       \
                                                                                                                \
typedef struct {                                                                                                \
    KeyT key;                                                                                                   \
    ValT value;                                                                                                 \
    zmap_state state;                                                                                          \
} zmap_bucket_##Name;                                                                                           \
                                                                                                                \
typedef struct {                                                                                                \
    zmap_bucket_##Name *buckets;                                                                                \
    size_t capacity;                                                                                            \
    size_t count;                                                                                               \
    size_t occupied;                                                                                            \
    uint32_t (*hash_func)(KeyT);                                                                                \
    int      (*cmp_func)(KeyT, KeyT);                                                                           \
} map_##Name;                                                                                                   \
                                                                                                                \
static inline void map_free_##Name(map_##Name *m) {                                                             \
    Z_MAP_FREE(m->buckets);                                                                                     \
    *m = (map_##Name){0};                                                                                       \
}                                                                                                               \
                                                                                                                \
static inline map_##Name map_init_##Name(uint32_t (*h)(KeyT), int (*c)(KeyT, KeyT)) {                           \
    return (map_##Name){ .hash_func = h, .cmp_func = c };                                                       \
}                                                                                                               \
                                                                                                                \
static inline int map_resize_##Name(map_##Name *m, size_t new_cap) {                                            \
    zmap_bucket_##Name *new_buckets = Z_MAP_CALLOC(new_cap, sizeof(zmap_bucket_##Name));                        \
    if (!new_buckets) return Z_ERR;                                                                             \
                                                                                                                \
    for (size_t i = 0; i < m->capacity; i++) {                                                                  \
            if (m->buckets[i].state == ZMAP_OCCUPIED) {                                                         \
                uint32_t hash = m->hash_func(m->buckets[i].key);                                                \
                size_t idx = hash % new_cap;                                                                    \
                while (new_buckets[idx].state == ZMAP_OCCUPIED) {                                               \
                    idx = (idx + 1) % new_cap;                                                                  \
                }                                                                                               \
                new_buckets[idx] = m->buckets[i];                                                               \
            }                                                                                                   \
        }                                                                                                       \
        Z_MAP_FREE(m->buckets);                                                                                 \
        m->buckets = new_buckets;                                                                               \
        m->capacity = new_cap;                                                                                  \
        m->occupied = m->count;                                                                                 \
        return Z_OK;                                                                                            \
}                                                                                                               \
                                                                                                                \
static inline int map_put_##Name(map_##Name *m, KeyT key, ValT val) {                                           \
        if (m->occupied >= m->capacity * 0.75) {                                                                \
            size_t new_cap = m->capacity == 0 ? 16 : m->capacity * 2;                                           \
            if (map_resize_##Name(m, new_cap) != Z_OK) return Z_ERR;                                            \
        }                                                                                                       \
        uint32_t hash = m->hash_func(key);                                                                      \
        size_t idx = hash % m->capacity;                                                                        \
        size_t deleted_idx = SIZE_MAX;                                                                          \
                                                                                                                \
        for (size_t i = 0; i < m->capacity; i++) {                                                              \
            zmap_state s = m->buckets[idx].state;                                                               \
            if (s == ZMAP_EMPTY) {                                                                              \
                if (deleted_idx != SIZE_MAX) idx = deleted_idx;                                                 \
                else m->occupied++;                                                                             \
                m->buckets[idx] = (zmap_bucket_##Name){ .key = key, .value = val, .state = ZMAP_OCCUPIED };     \
                m->count++;                                                                                     \
                return Z_OK;                                                                                    \
            }                                                                                                   \
            if (s == ZMAP_DELETED) {                                                                            \
                if (deleted_idx == SIZE_MAX) deleted_idx = idx;                                                 \
            }                                                                                                   \
            else if (m->cmp_func(m->buckets[idx].key, key) == 0) {                                              \
                m->buckets[idx].value = val;                                                                    \
                return Z_OK;                                                                                    \
            }                                                                                                   \
            idx = (idx + 1) % m->capacity;                                                                      \
        }                                                                                                       \
        return Z_ERR;                                                                                           \
    }                                                                                                           \
                                                                                                                \
    static inline ValT* map_get_##Name(map_##Name *m, KeyT key) {                                               \
        if (m->count == 0) return NULL;                                                                         \
        uint32_t hash = m->hash_func(key);                                                                      \
        size_t idx = hash % m->capacity;                                                                        \
                                                                                                                \
        for (size_t i = 0; i < m->capacity; i++) {                                                              \
            zmap_state s = m->buckets[idx].state;                                                               \
            if (s == ZMAP_EMPTY) return NULL;                                                                   \
            if (s == ZMAP_OCCUPIED && m->cmp_func(m->buckets[idx].key, key) == 0) {                             \
                return &m->buckets[idx].value;                                                                  \
            }                                                                                                   \
            idx = (idx + 1) % m->capacity;                                                                      \
        }                                                                                                       \
        return NULL;                                                                                            \
    }                                                                                                           \
                                                                                                                \
    static inline void map_remove_##Name(map_##Name *m, KeyT key) {                                             \
        if (m->count == 0) return;                                                                              \
        uint32_t hash = m->hash_func(key);                                                                      \
        size_t idx = hash % m->capacity;                                                                        \
                                                                                                                \
        for (size_t i = 0; i < m->capacity; i++) {                                                              \
            zmap_state s = m->buckets[idx].state;                                                               \
            if (s == ZMAP_EMPTY) return;                                                                        \
            if (s == ZMAP_OCCUPIED && m->cmp_func(m->buckets[idx].key, key) == 0) {                             \
                m->buckets[idx].state = ZMAP_DELETED;                                                           \
                m->count--;                                                                                     \
                return;                                                                                         \
            }                                                                                                   \
            idx = (idx + 1) % m->capacity;                                                                      \
        }                                                                                                       \
    }                                                                                                           \
                                                                                                                \
    static inline size_t map_size_##Name(map_##Name *m) { return m->count; }                                    \
                                                                                                                \
    static inline void map_clear_##Name(map_##Name *m) {                                                        \
        if (m->capacity > 0) {                                                                                  \
             memset(m->buckets, 0, m->capacity * sizeof(zmap_bucket_##Name));                                   \
        }                                                                                                       \
        m->count = 0;                                                                                           \
        m->occupied = 0;                                                                                        \
    }

#define M_PUT_ENTRY(K, V, N)    map_##N*: map_put_##N,
#define M_GET_ENTRY(K, V, N)    map_##N*: map_get_##N,
#define M_REM_ENTRY(K, V, N)    map_##N*: map_remove_##N,
#define M_FREE_ENTRY(K, V, N)   map_##N*: map_free_##N,
#define M_SIZE_ENTRY(K, V, N)   map_##N*: map_size_##N,
#define M_CLEAR_ENTRY(K, V, N)  map_##N*: map_clear_##N,

#define map_init(Name, h_func, c_func) map_init_##Name(h_func, c_func)

#if defined(Z_HAS_CLEANUP) && Z_HAS_CLEANUP
    #define map_autofree(Name)  Z_CLEANUP(map_free_##Name) map_##Name
#endif

#define map_put(m, k, v)   _Generic((m), REGISTER_MAP_TYPES(M_PUT_ENTRY)  default: 0) (m, k, v)
#define map_get(m, k)      _Generic((m), REGISTER_MAP_TYPES(M_GET_ENTRY)  default: (void*)0) (m, k)
#define map_remove(m, k)   _Generic((m), REGISTER_MAP_TYPES(M_REM_ENTRY)  default: (void)0) (m, k)
#define map_free(m)        _Generic((m), REGISTER_MAP_TYPES(M_FREE_ENTRY) default: (void)0) (m)
#define map_size(m)        _Generic((m), REGISTER_MAP_TYPES(M_SIZE_ENTRY) default: 0) (m)
#define map_clear(m)       _Generic((m), REGISTER_MAP_TYPES(M_CLEAR_ENTRY) default: (void)0) (m)

#endif
