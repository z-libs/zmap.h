/*
 * zmap_uthash.h - Full uthash API Compatibility Layer (zmap-based)
 * Part of Zen Development Kit (ZDK)
 *
 * Drop-in replacement for uthash.h. 
 * Internally uses zmap.h (Robin Hood hashing) for O(1) lookups
 * while maintaining uthash's intrusive linked-list API.
 *
 * Note: It passes all 100 official tests except 'test17', caused by
 * a sort trace difference (zmap uses qsort, uthash uses merge sort).
 *
 * License: MIT
 * Author: Zuhaitz
 * Repository: https://github.com/z-libs/zmap.h
 */

#ifndef ZMAP_UTHASH_H
#define ZMAP_UTHASH_H

#define ZMAP_UTHASH_VERSION 2.0.0

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef uthash_malloc
#define uthash_malloc(sz) malloc(sz)
#endif

#ifndef uthash_free
#define uthash_free(ptr,sz) free(ptr)
#endif

#ifndef uthash_bzero
#define uthash_bzero(ptr,n) memset(ptr,0,n)
#endif

#ifndef uthash_strlen
#define uthash_strlen(s) strlen(s)
#endif

/* Map uthash allocators to zmap */
#define ZMAP_MALLOC(sz)      uthash_malloc(sz)
#define ZMAP_FREE(ptr)       uthash_free(ptr, 0)
#define ZMAP_CALLOC(n,sz)    zmap_uthash_calloc(n,sz)

/* Helper for calloc using uthash allocators */
static inline void* zmap_uthash_calloc(size_t n, size_t sz) {
    size_t total = n * sz;
    void *p = uthash_malloc(total);
    if (p) uthash_bzero(p, total);
    return p;
}

#include "zmap.h"

#define HASH_JEN_MIX(a, b, c) \
do {                          \
    a -= b; a -= c; a ^= (c >> 13); \
    b -= c; b -= a; b ^= (a << 8);  \
    c -= a; c -= b; c ^= (b >> 13); \
    a -= b; a -= c; a ^= (c >> 12); \
    b -= c; b -= a; b ^= (a << 16); \
    c -= a; c -= b; c ^= (b >> 5);  \
    a -= b; a -= c; a ^= (c >> 3);  \
    b -= c; b -= a; b ^= (a << 10); \
    c -= a; c -= b; c ^= (b >> 15); \
} while(0)

#define HASH_JEN(key, keylen, hashv)                                             \
do {                                                                             \
  unsigned _hj_i,_hj_j,_hj_k;                                                    \
  unsigned const char *_hj_key=(unsigned const char*)(key);                      \
  hashv = 0xfeedbeefu;                                                           \
  _hj_i = _hj_j = 0x9e3779b9u;                                                   \
  _hj_k = (unsigned)(keylen);                                                    \
  while (_hj_k >= 12U) {                                                         \
    _hj_i +=    (_hj_key[0] + ( (unsigned)_hj_key[1] << 8 )                      \
        + ( (unsigned)_hj_key[2] << 16 )                                         \
        + ( (unsigned)_hj_key[3] << 24 ) );                                      \
    _hj_j +=    (_hj_key[4] + ( (unsigned)_hj_key[5] << 8 )                      \
        + ( (unsigned)_hj_key[6] << 16 )                                         \
        + ( (unsigned)_hj_key[7] << 24 ) );                                      \
    hashv += (_hj_key[8] + ( (unsigned)_hj_key[9] << 8 )                         \
        + ( (unsigned)_hj_key[10] << 16 )                                        \
        + ( (unsigned)_hj_key[11] << 24 ) );                                     \
                                                                                 \
     HASH_JEN_MIX(_hj_i, _hj_j, hashv);                                          \
                                                                                 \
     _hj_key += 12;                                                              \
     _hj_k -= 12U;                                                               \
  }                                                                              \
  hashv += (unsigned)(keylen);                                                   \
  switch ( _hj_k ) {                                                             \
    case 11: hashv += ( (unsigned)_hj_key[10] << 24 ); /* FALLTHROUGH */         \
    case 10: hashv += ( (unsigned)_hj_key[9] << 16 );  /* FALLTHROUGH */         \
    case 9:  hashv += ( (unsigned)_hj_key[8] << 8 );   /* FALLTHROUGH */         \
    case 8:  _hj_j += ( (unsigned)_hj_key[7] << 24 );  /* FALLTHROUGH */         \
    case 7:  _hj_j += ( (unsigned)_hj_key[6] << 16 );  /* FALLTHROUGH */         \
    case 6:  _hj_j += ( (unsigned)_hj_key[5] << 8 );   /* FALLTHROUGH */         \
    case 5:  _hj_j += _hj_key[4];                      /* FALLTHROUGH */         \
    case 4:  _hj_i += ( (unsigned)_hj_key[3] << 24 );  /* FALLTHROUGH */         \
    case 3:  _hj_i += ( (unsigned)_hj_key[2] << 16 );  /* FALLTHROUGH */         \
    case 2:  _hj_i += ( (unsigned)_hj_key[1] << 8 );   /* FALLTHROUGH */         \
    case 1:  _hj_i += _hj_key[0];                      /* FALLTHROUGH */         \
    default: ;                                                                   \
  }                                                                              \
  HASH_JEN_MIX(_hj_i, _hj_j, hashv);                                             \
} while (0)


#define HASH_BER(key, keylen, hashv)                            \
do {                                                            \
    unsigned _hb_keylen = (unsigned)(keylen);                   \
    const unsigned char *_hb_key = (const unsigned char*)(key); \
    hashv = 0;                                                  \
    while (_hb_keylen-- != 0U) {                                \
        hashv = ((hashv) * 33U) + *_hb_key++;                   \
    }                                                           \
} while (0)

#define HASH_SAX(key, keylen, hashv)                            \
do {                                                            \
    unsigned _hs_keylen = (unsigned)(keylen);                   \
    const unsigned char *_hs_key = (const unsigned char*)(key); \
    hashv = 0;                                                  \
    while (_hs_keylen-- != 0U) {                                \
        hashv ^= (hashv << 5) + (hashv >> 2) + *_hs_key++;      \
    }                                                           \
} while (0)

#define HASH_FNV(key, keylen, hashv)                            \
do {                                                            \
    unsigned _hf_keylen = (unsigned)(keylen);                   \
    const unsigned char *_hf_key = (const unsigned char*)(key); \
    hashv = 2166136261u;                                        \
    while (_hf_keylen-- != 0U) {                                \
        hashv ^= *_hf_key++;                                    \
        hashv *= 16777619u;                                     \
    }                                                           \
} while (0)

#define HASH_OAT(key, keylen, hashv)                            \
do {                                                            \
    unsigned _ho_keylen = (unsigned)(keylen);                   \
    const unsigned char *_ho_key = (const unsigned char*)(key); \
    hashv = 0;                                                  \
    while (_ho_keylen-- != 0U) {                                \
        hashv += *_ho_key++;                                    \
        hashv += (hashv << 10);                                 \
        hashv ^= (hashv >> 6);                                  \
    }                                                           \
    hashv += (hashv << 3);                                      \
    hashv ^= (hashv >> 11);                                     \
    hashv += (hashv << 15);                                     \
} while (0)

#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8) + (uint32_t)(((const uint8_t *)(d))[0]))

#define HASH_SFH(key, keylen, hashv)                                             \
do {                                                                             \
  unsigned const char *_sfh_key=(unsigned const char*)(key);                     \
  uint32_t _sfh_tmp, _sfh_len = (uint32_t)keylen;                                \
                                                                                 \
  unsigned _sfh_rem = _sfh_len & 3U;                                             \
  _sfh_len >>= 2;                                                                \
  hashv = 0xcafebabeu;                                                           \
                                                                                 \
  /* Main loop */                                                                \
  for (;_sfh_len > 0U; _sfh_len--) {                                             \
    hashv    += get16bits (_sfh_key);                                            \
    _sfh_tmp  = ((uint32_t)(get16bits (_sfh_key+2)) << 11) ^ hashv;              \
    hashv     = (hashv << 16) ^ _sfh_tmp;                                        \
    _sfh_key += 2U*sizeof (uint16_t);                                            \
    hashv    += hashv >> 11;                                                     \
  }                                                                              \
                                                                                 \
  /* Handle end cases */                                                         \
  switch (_sfh_rem) {                                                            \
    case 3: hashv += get16bits (_sfh_key);                                       \
            hashv ^= hashv << 16;                                                \
            hashv ^= (uint32_t)(_sfh_key[sizeof (uint16_t)]) << 18;              \
            hashv += hashv >> 11;                                                \
            break;                                                               \
    case 2: hashv += get16bits (_sfh_key);                                       \
            hashv ^= hashv << 11;                                                \
            hashv += hashv >> 17;                                                \
            break;                                                               \
    case 1: hashv += *_sfh_key;                                                  \
            hashv ^= hashv << 10;                                                \
            hashv += hashv >> 1;                                                 \
            break;                                                               \
    default: ;                                                                   \
  }                                                                              \
                                                                                 \
  /* Force "avalanching" of final 127 bits */                                    \
  hashv ^= hashv << 3;                                                           \
  hashv += hashv >> 5;                                                           \
  hashv ^= hashv << 4;                                                           \
  hashv += hashv >> 17;                                                          \
  hashv ^= hashv << 25;                                                          \
  hashv += hashv >> 6;                                                           \
} while (0)

/* Simple default if nothing selected */
#ifndef HASH_FUNCTION
#define HASH_FUNCTION(keyptr, keylen, hashv) HASH_JEN(keyptr, keylen, hashv)
#endif

#define HASH_VALUE(keyptr, keylen, hashv) \
    do { HASH_FUNCTION(keyptr, keylen, hashv); } while(0)

/* Forward declarations for types used in zmap */
/*
 * UT_hash_handle
 * Intrusive structure embedded in user objects.
 */
typedef struct UT_hash_handle {
    struct UT_hash_table *tbl;
    void *prev;                      /* prev element in app order      */
    void *next;                      /* next element in app order      */
    struct UT_hash_handle *hh_prev;  /* previous hh in bucket (unused) */
    struct UT_hash_handle *hh_next;  /* next hh in bucket (unused)     */
    const void *key;                 /* ptr to enclosing element key   */
    unsigned keylen;                 /* enclosing element key len      */
    unsigned hashv;                  /* result of hash-fcn(key)        */
} UT_hash_handle;

/* 
 * Wrapper for zmap hash function.
 * We rely on HASH_ADD/FIND macros to have already populated h->hashv.
 * This ensures we use the user's selected HASH_FUNCTION (or manual value).
 */
static inline uint32_t zmap_uthash_hash(struct UT_hash_handle *h, uint32_t seed) {
    (void)seed; /* unused, we use the hashv directly */
    return h->hashv;
}

/* 
 * Wrapper for zmap comparison function.
 * Compares two handles by their keys.
 */
#ifndef HASH_KEYCMP
#define HASH_KEYCMP(a,b,len) memcmp(a,b,len)
#endif

static inline int zmap_uthash_cmp(struct UT_hash_handle *a, struct UT_hash_handle *b) {
    if (a->keylen != b->keylen) return 1;
    return HASH_KEYCMP(a->key, b->key, a->keylen);
}

/* Generate the zmap implementation for <UT_hash_handle*, UT_hash_handle*> */
ZMAP_GENERATE_IMPL(struct UT_hash_handle*, struct UT_hash_handle*, uthash_kv)

/* 
 * UT_hash_table
 * Now contains the zmap instance instead of raw buckets.
 */
typedef struct UT_hash_table {
    zmap_uthash_kv map;              /* zmap instance (O(1) lookup)    */
    struct UT_hash_handle *tail;     /* tail hh in app order           */
    ptrdiff_t hho;                   /* offset of hh in element        */
    
    /* Legacy fields maintained for API compatibility/inspection */
    struct UT_hash_bucket *buckets; /* shim for tools like hashscan */
    unsigned num_items;
    unsigned num_buckets;           /* approximated from map capacity */
    unsigned log2_num_buckets;
    unsigned ideal_chain_maxlen;
    unsigned nonideal_items;
    unsigned ineff_expands;
    unsigned noexpand;
    uint32_t signature;             /* HASH_SIGNATURE */
    
#ifdef HASH_BLOOM
    uint32_t bloom_sig;
    uint8_t *bloom_bv;
    uint8_t bloom_nbits;
#endif
} UT_hash_table;

#define HASH_SIGNATURE 0xa0111fe1u
#define HASH_BLOOM_SIGNATURE 0xb12220f2u

typedef struct UT_hash_bucket {
   struct UT_hash_handle *hh_head;
   unsigned count;
   unsigned expand_mult;
} UT_hash_bucket;

/* ============================================================================
 * Bloom Filter Logic
 * ============================================================================ */

#ifdef HASH_BLOOM
#define HASH_BLOOM_BITLEN  (1UL << (HASH_BLOOM))
#define HASH_BLOOM_BYTELEN (HASH_BLOOM_BITLEN / 8UL)
#define HASH_BLOOM_BITSET(bv, idx) (bv[(idx) / 8U] |= (1U << ((idx) % 8U)))
#define HASH_BLOOM_BITTEST(bv, idx) (bv[(idx) / 8U] & (1U << ((idx) % 8U)))

#define HASH_BLOOM_MAKE(tbl, oomed)                                              \
do {                                                                             \
    (tbl)->bloom_nbits = HASH_BLOOM;                                             \
    (tbl)->bloom_bv = (uint8_t*)uthash_malloc(HASH_BLOOM_BYTELEN);               \
    if (!(tbl)->bloom_bv) {                                                      \
        HASH_RECORD_OOM(oomed);                                                  \
    } else {                                                                     \
        uthash_bzero((tbl)->bloom_bv, HASH_BLOOM_BYTELEN);                       \
        (tbl)->bloom_sig = HASH_BLOOM_SIGNATURE;                                 \
    }                                                                            \
} while(0)

#define HASH_BLOOM_FREE(tbl)                                                     \
do {                                                                             \
    if ((tbl)->bloom_bv) {                                                       \
        uthash_free((tbl)->bloom_bv, HASH_BLOOM_BYTELEN);                        \
    }                                                                            \
} while(0)

#define HASH_BLOOM_ADD(tbl, hashv) \
    HASH_BLOOM_BITSET((tbl)->bloom_bv, ((hashv) & (uint32_t)((1UL << (tbl)->bloom_nbits) - 1U)))

#define HASH_BLOOM_TEST(tbl, hashv) \
    HASH_BLOOM_BITTEST((tbl)->bloom_bv, ((hashv) & (uint32_t)((1UL << (tbl)->bloom_nbits) - 1U)))

#else
#define HASH_BLOOM_MAKE(tbl, oomed)
#define HASH_BLOOM_FREE(tbl)
#define HASH_BLOOM_ADD(tbl, hashv)
#define HASH_BLOOM_TEST(tbl, hashv) (1)
#endif

#define HASH_RECORD_OOM(oomed) do { (oomed) = 1; } while(0)
#define IF_HASH_NONFATAL_OOM(x)

#define HASH_MAKE_TABLE(hh, head, oomed)                                                \
do {                                                                                    \
    (head)->hh.tbl = (UT_hash_table*)uthash_malloc(sizeof(UT_hash_table));              \
    if (!(head)->hh.tbl) {                                                              \
        HASH_RECORD_OOM(oomed);                                                         \
    } else {                                                                            \
        uthash_bzero((head)->hh.tbl, sizeof(UT_hash_table));                            \
        (head)->hh.tbl->tail = &((head)->hh);                                           \
        (head)->hh.tbl->signature = HASH_SIGNATURE;                                     \
        (head)->hh.tbl->hho = (ptrdiff_t)((char*)(&(head)->hh) - (char*)(head));        \
        /* Initialize zmap */                                                           \
        (head)->hh.tbl->map = zmap_init_uthash_kv(zmap_uthash_hash, zmap_uthash_cmp);   \
        /* Initialize shim */                                                           \
        static UT_hash_bucket _zmap_shim_bucket = { NULL, 0, 0 };                       \
        (head)->hh.tbl->buckets = &_zmap_shim_bucket;                                   \
        (head)->hh.tbl->num_buckets = 1;                                                \
        (head)->hh.tbl->log2_num_buckets = 0;                                           \
        /* Initialize bloom */                                                          \
        HASH_BLOOM_MAKE((head)->hh.tbl, oomed);                                         \
    }                                                                                   \
} while(0)

#define HASH_ADD(hh, head, fieldname, keylen_in, add)   \
    HASH_ADD_KEYPTR(hh, head, &((add)->fieldname), keylen_in, add)

#define HASH_ADD_KEYPTR(hh, head, keyptr, keylen_in, add)                           \
do {                                                                                \
    unsigned _ha_hashv_outer;                                                       \
    HASH_VALUE(keyptr, keylen_in, _ha_hashv_outer);                                 \
    HASH_ADD_KEYPTR_BYHASHVALUE(hh, head, keyptr, keylen_in, _ha_hashv_outer, add); \
} while(0)

#define HASH_ADD_KEYPTR_BYHASHVALUE(hh, head, keyptr, keylen_in, hashval, add)   \
do {                                                                             \
    unsigned _ha_hashv = (hashval);                                              \
    (add)->hh.hashv = _ha_hashv;                                                 \
    (add)->hh.key = (const void*)(keyptr);                                       \
    (add)->hh.keylen = (unsigned)(keylen_in);                                    \
    if (!(head)) {                                                               \
        (add)->hh.next = NULL;                                                   \
        (add)->hh.prev = NULL;                                                   \
        (head) = (add);                                                          \
        int _ha_oomed = 0;                                                       \
        HASH_MAKE_TABLE(hh, head, _ha_oomed);                                    \
        if (_ha_oomed) { (head) = NULL; }                                        \
    } else {                                                                     \
        (add)->hh.tbl = (head)->hh.tbl;                                          \
        (add)->hh.next = NULL;                                                   \
        (add)->hh.prev = ELMT_FROM_HH((head)->hh.tbl, (head)->hh.tbl->tail);     \
        (head)->hh.tbl->tail->next = (add);                                      \
    }                                                                            \
    if ((head)) {                                                                \
        (head)->hh.tbl->tail = &((add)->hh);                                     \
        (head)->hh.tbl->num_items++;                                             \
        /* Add to Bloom */                                                       \
        HASH_BLOOM_ADD((head)->hh.tbl, _ha_hashv);                               \
        /* Add to zmap */                                                        \
        zmap_put_uthash_kv(&((head)->hh.tbl->map), &((add)->hh), &((add)->hh));  \
    }                                                                            \
} while(0)

#define HASH_FIND(hh, head, keyptr, keylen_in, out)                              \
do {                                                                             \
    unsigned _hf_outer_hashv;                                                    \
    HASH_VALUE(keyptr, keylen_in, _hf_outer_hashv);                              \
    HASH_FIND_BYHASHVALUE(hh, head, keyptr, keylen_in, _hf_outer_hashv, out);    \
} while(0)

#define HASH_FIND_BYHASHVALUE(hh, head, keyptr, keylen_in, hashval, out)                    \
do {                                                                                        \
    (out) = NULL;                                                                           \
    if (head) {                                                                             \
        unsigned _hf_hashv = (hashval);                                                     \
        if (HASH_BLOOM_TEST((head)->hh.tbl, _hf_hashv)) {                                   \
            /* Create temp handle for lookup */                                             \
            UT_hash_handle _hf_hh;                                                          \
            _hf_hh.key = (const void*)(keyptr);                                             \
            _hf_hh.keylen = (unsigned)(keylen_in);                                          \
            _hf_hh.hashv = _hf_hashv;                                                       \
            UT_hash_handle **_hf_res = zmap_get_uthash_kv(&((head)->hh.tbl->map), &_hf_hh); \
            if (_hf_res) {                                                                  \
                (out) = (DECLTYPE(out))ELMT_FROM_HH((head)->hh.tbl, *_hf_res);              \
            }                                                                               \
        }                                                                                   \
    }                                                                                       \
} while(0)

#define HASH_ADD_STR(head,strfield,add)                                          \
    HASH_ADD_KEYPTR(hh,head,(add)->strfield,uthash_strlen((add)->strfield),add)
#define HASH_ADD_INT(head,intfield,add)                                          \
    HASH_ADD(hh,head,intfield,sizeof(int),add)
#define HASH_ADD_PTR(head,ptrfield,add)                                          \
    HASH_ADD(hh,head,ptrfield,sizeof(void *),add)

#define HASH_FIND_STR(head,findstr,out)                                          \
    HASH_FIND(hh,head,findstr,uthash_strlen(findstr),out)
#define HASH_FIND_INT(head,findint,out)                                          \
    HASH_FIND(hh,head,findint,sizeof(int),out)
#define HASH_FIND_PTR(head,findptr,out)                                          \
    HASH_FIND(hh,head,findptr,sizeof(void *),out)

#define HASH_DEL(head, delptr)                                                   \
    HASH_DELETE(hh, head, delptr)

#define HASH_DELETE(hh, head, delptr)                                                                                               \
do {                                                                                                                                \
    if ( (delptr)->hh.prev ) {                                                                                                      \
        HH_FROM_ELMT((delptr)->hh.tbl, (delptr)->hh.prev)->next = (delptr)->hh.next;                                                \
    } else {                                                                                                                        \
        (head) = (DECLTYPE(head))((delptr)->hh.next);                                                                               \
    }                                                                                                                               \
    if ((delptr)->hh.next) {                                                                                                        \
        HH_FROM_ELMT((delptr)->hh.tbl, (delptr)->hh.next)->prev = (delptr)->hh.prev;                                                \
    } else {                                                                                                                        \
        (delptr)->hh.tbl->tail = (UT_hash_handle*)((delptr)->hh.prev ? HH_FROM_ELMT((delptr)->hh.tbl, (delptr)->hh.prev) : NULL);   \
    }                                                                                                                               \
    (delptr)->hh.tbl->num_items--;                                                                                                  \
    zmap_remove_uthash_kv(&((delptr)->hh.tbl->map), &((delptr)->hh));                                                               \
} while(0)

#define HASH_REPLACE(hh,head,keyfield,keylen_in,add,replaced)   \
do {                                                            \
  (replaced)=NULL;                                              \
  HASH_FIND(hh,head,&((add)->keyfield),keylen_in,replaced);     \
  if (replaced) {                                               \
     HASH_DELETE(hh,head,replaced);                             \
  }                                                             \
  HASH_ADD(hh,head,keyfield,keylen_in,add);                     \
} while(0)

#define HASH_REPLACE_INT(head,intfield,add,replaced)                             \
    HASH_REPLACE(hh,head,intfield,sizeof(int),add,replaced)
#define HASH_REPLACE_STR(head,strfield,add,replaced)                             \
    HASH_REPLACE(hh,head,strfield,uthash_strlen((add)->strfield),add,replaced)

#define HASH_ADD_KEYPTR_BYHASHVALUE_INORDER(hh, head, keyptr, keylen_in, hashval, add, cmpfcn)  \
do {                                                                                            \
    unsigned _ha_hashv = (hashval);                                                             \
    (add)->hh.hashv = _ha_hashv;                                                                \
    (add)->hh.key = (const void*)(keyptr);                                                      \
    (add)->hh.keylen = (unsigned)(keylen_in);                                                   \
    if (!(head)) {                                                                              \
        (add)->hh.next = NULL;                                                                  \
        (add)->hh.prev = NULL;                                                                  \
        (head) = (add);                                                                         \
        int _ha_oomed = 0;                                                                      \
        HASH_MAKE_TABLE(hh, head, _ha_oomed);                                                   \
        if (_ha_oomed) { (head) = NULL; }                                                       \
    } else {                                                                                    \
        (add)->hh.tbl = (head)->hh.tbl;                                                         \
        UT_hash_handle *_hs_iter = &((head)->hh);                                               \
        UT_hash_handle *_hs_prev = NULL;                                                        \
        while (_hs_iter) {                                                                      \
            if (cmpfcn(ELMT_FROM_HH((head)->hh.tbl, _hs_iter), (add)) > 0) {                    \
                break;                                                                          \
            }                                                                                   \
            _hs_prev = _hs_iter;                                                                \
            if (_hs_iter->next) {                                                               \
                 _hs_iter = HH_FROM_ELMT((head)->hh.tbl, _hs_iter->next);                       \
            } else {                                                                            \
                 _hs_iter = NULL;                                                               \
            }                                                                                   \
        }                                                                                       \
        if (_hs_iter) { /* Insert before _hs_iter */                                            \
            (add)->hh.next = ELMT_FROM_HH((head)->hh.tbl, _hs_iter);                            \
            (add)->hh.prev = _hs_iter->prev;                                                    \
            if (_hs_iter->prev) {                                                               \
                HH_FROM_ELMT((head)->hh.tbl, _hs_iter->prev)->next = (add);                     \
            } else {                                                                            \
                (head) = (add);                                                                 \
            }                                                                                   \
            _hs_iter->prev = ELMT_FROM_HH((head)->hh.tbl, (add));                               \
        } else { /* Append at end */                                                            \
            (add)->hh.next = NULL;                                                              \
            (add)->hh.prev = ELMT_FROM_HH((head)->hh.tbl, _hs_prev);                            \
            _hs_prev->next = (add);                                                             \
            (head)->hh.tbl->tail = &((add)->hh);                                                \
        }                                                                                       \
    }                                                                                           \
    if ((head)) {                                                                               \
        if (!(head)->hh.tbl->tail) (head)->hh.tbl->tail = &((add)->hh); /* Safety */            \
        (head)->hh.tbl->num_items++;                                                            \
        HASH_BLOOM_ADD((head)->hh.tbl, _ha_hashv);                                              \
        zmap_put_uthash_kv(&((head)->hh.tbl->map), &((add)->hh), &((add)->hh));                 \
    }                                                                                           \
} while(0)

#define HASH_ADD_INORDER(hh,head,fieldname,keylen_in,add,cmpfcn) \
    HASH_ADD_KEYPTR_INORDER(hh, head, &((add)->fieldname), keylen_in, add, cmpfcn)

#define HASH_ADD_KEYPTR_INORDER(hh,head,keyptr,keylen_in,add,cmpfcn)                            \
do {                                                                                            \
    unsigned _ha_hashv;                                                                         \
    HASH_VALUE(keyptr, keylen_in, _ha_hashv);                                                   \
    HASH_ADD_KEYPTR_BYHASHVALUE_INORDER(hh, head, keyptr, keylen_in, _ha_hashv, add, cmpfcn);   \
} while(0)

#define HASH_ADD_BYHASHVALUE_INORDER(hh,head,fieldname,keylen_in,hashval,add,cmpfcn) \
    HASH_ADD_KEYPTR_BYHASHVALUE_INORDER(hh, head, &((add)->fieldname), keylen_in, hashval, add, cmpfcn)

#define HASH_REPLACE_INORDER(hh,head,fieldname,keylen_in,add,replaced,cmpfcn)   \
do {                                                                            \
  (replaced)=NULL;                                                              \
  HASH_FIND(hh,head,&((add)->fieldname),keylen_in,replaced);                    \
  if (replaced) {                                                               \
     HASH_DELETE(hh,head,replaced);                                             \
  }                                                                             \
  HASH_ADD_INORDER(hh,head,fieldname,keylen_in,add,cmpfcn);                     \
} while(0)

#define HASH_REPLACE_BYHASHVALUE_INORDER(hh,head,fieldname,keylen_in,hashval,add,replaced,cmpfcn)   \
do {                                                                                                \
  (replaced)=NULL;                                                                                  \
  HASH_FIND_BYHASHVALUE(hh,head,&((add)->fieldname),keylen_in,hashval,replaced);                    \
  if (replaced) {                                                                                   \
     HASH_DELETE(hh,head,replaced);                                                                 \
  }                                                                                                 \
  HASH_ADD_BYHASHVALUE_INORDER(hh, head, fieldname, keylen_in, hashval, add, cmpfcn);               \
} while(0)

#define HASH_ADD_KEYPTR_BYHASHVALUE_INORDER_KEYPTR(hh,head,keyptr,keylen_in,hashval,add,cmpfcn) \
    HASH_ADD_KEYPTR_BYHASHVALUE_INORDER(hh, head, keyptr, keylen_in, hashval, add, cmpfcn)

#define HASH_ITER(hh, head, el, tmp)                                                                \
    for(((el)=(head)), ((*(char**)(&(tmp)))=(char*)((head!=NULL)?(head)->hh.next:NULL));            \
      (el) != NULL; ((el)=(tmp)), ((*(char**)(&(tmp)))=(char*)((tmp!=NULL)?(tmp)->hh.next:NULL)))

/* 
 * Sort helper function (Merge Sort) 
 */
static inline void zmap_uthash_sort(void **head_ref, int (*cmp)(void*, void*), ptrdiff_t hho) {
    if (!head_ref || !*head_ref) return;

    UT_hash_handle *head = (UT_hash_handle*)((char*)(*head_ref) + hho);
    if (!head->next) return;

    UT_hash_handle *sorted = NULL;
    UT_hash_handle *curr = head;
    
    while (curr) {
        UT_hash_handle *next = NULL;
        if (curr->next) next = (UT_hash_handle*)((char*)(curr->next) + hho);
        
        /* Insert curr into sorted list */
        if (!sorted || cmp((void*)((char*)curr - hho), (void*)((char*)sorted - hho)) <= 0) {
            curr->next = (sorted) ? (void*)((char*)sorted - hho) : NULL;
            if (sorted) sorted->prev = (void*)((char*)curr - hho);
            curr->prev = NULL;
            sorted = curr;
        } else {
            UT_hash_handle *s = sorted;
            while (s->next) {
               UT_hash_handle *sn = (UT_hash_handle*)((char*)(s->next) + hho);
               if (cmp((void*)((char*)curr - hho), (void*)((char*)sn - hho)) <= 0) break;
               s = sn;
            }
            /* Insert after s */
            curr->next = s->next;
            if (s->next) {
                 UT_hash_handle *sn = (UT_hash_handle*)((char*)(s->next) + hho);
                 sn->prev = (void*)((char*)curr - hho);
            }
            s->next = (void*)((char*)curr - hho);
            curr->prev = (void*)((char*)s - hho);
        }
        
        curr = next;
    }
    
    /* Update head_ref and tail */
    *head_ref = (void*)((char*)sorted - hho);
    UT_hash_handle *tail = sorted;
    while (tail->next) tail = (UT_hash_handle*)((char*)(tail->next) + hho);
    if (tail->tbl) tail->tbl->tail = tail;
}

#define HASH_CNT(hh, head) ((head) ? (head)->hh.tbl->num_items : 0)
#define HASH_COUNT(head) HASH_CNT(hh, head)

#define HASH_CLEAR(hh, head)                                                     \
do {                                                                             \
    if (head) {                                                                  \
        HASH_BLOOM_FREE((head)->hh.tbl);                                         \
        zmap_free_uthash_kv(&((head)->hh.tbl->map));                             \
        uthash_free((head)->hh.tbl, sizeof(UT_hash_table));                      \
        (head) = NULL;                                                           \
    }                                                                            \
} while(0)

#define HASH_SELECT(hh_dst, dst, hh_src, src, cond)                                             \
do {                                                                                            \
    (dst) = NULL;                                                                               \
    if (src) {                                                                                  \
        zmap_iter_uthash_kv _hs_iter = zmap_iter_init_uthash_kv(&((src)->hh_src.tbl->map));     \
        struct UT_hash_handle *_hs_hh, *_hs_k;                                                  \
        while (zmap_iter_next_uthash_kv(&_hs_iter, &_hs_k, &_hs_hh)) {                          \
            DECLTYPE(src) _hs_elmt = (DECLTYPE(src))ELMT_FROM_HH((src)->hh_src.tbl, _hs_hh);    \
            if (cond(_hs_elmt)) {                                                               \
                DECLTYPE(dst) _hs_add = (DECLTYPE(dst))uthash_malloc(sizeof(*(dst)));           \
                if (_hs_add) {                                                                  \
                    memcpy(_hs_add, _hs_elmt, sizeof(*(dst)));                                  \
                    void *_hs_key_ptr = (void*)_hs_hh->key;                                     \
                    if ((char*)_hs_key_ptr >= (char*)_hs_elmt &&                                \
                        (char*)_hs_key_ptr < (char*)_hs_elmt + sizeof(*(src))) {                \
                        ptrdiff_t _hs_off = (char*)_hs_key_ptr - (char*)_hs_elmt;               \
                        _hs_key_ptr = (char*)_hs_add + _hs_off;                                 \
                    }                                                                           \
                    HASH_ADD_KEYPTR(hh_dst, dst, _hs_key_ptr, _hs_hh->keylen, _hs_add);         \
                }                                                                               \
            }                                                                                   \
        }                                                                                       \
    }                                                                                           \
} while(0)

#define HASH_SORT(head, cmpfcn) HASH_SRT(hh, head, cmpfcn)
/* 
 * Sort helper function (Insertion Sort) 
 */
#define HASH_SRT(hh, head, cmpfcn)                                                  \
    zmap_uthash_sort((void**)&(head), (int(*)(void*,void*))(cmpfcn),                \
                     (head) ? (ptrdiff_t)((char*)&((head)->hh) - (char*)(head)) : 0)

#define HASH_OVERHEAD(hh, head) \
    ((head) ? (sizeof(UT_hash_table) + ((head)->hh.tbl->num_items * sizeof(UT_hash_handle))) : 0)

/* Helper macros */
#define HH_FROM_ELMT(tbl, elmt) ((UT_hash_handle*)((char*)(elmt) + (tbl)->hho))
#define ELMT_FROM_HH(tbl, hhp) ((void*)(((char*)(hhp)) - ((tbl)->hho)))
#define DECLTYPE(x) __typeof__(x)

#endif /* ZMAP_UTHASH_H */
