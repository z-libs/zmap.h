#ifndef ZHASH_H
#define ZHASH_H

#include <stdint.h>
#include <string.h>
#include <stddef.h>

 /* WyHash implementation (adapted, this is for my own tests).
  * * Origin: https://github.com/wangyi-fudan/wyhash
*/
static inline uint64_t _zhash_read64(const uint8_t *p) { uint64_t v; memcpy(&v, p, 8); return v; }
static inline uint64_t _zhash_read32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }
static inline uint64_t _zhash_read24(const uint8_t *p) { return (((uint64_t)p[0]) << 16) | (((uint64_t)p[1]) << 8) | p[2]; }
static inline uint64_t _zhash_read16(const uint8_t *p) { uint16_t v; memcpy(&v, p, 2); return v; }
static inline uint64_t _zhash_read08(const uint8_t *p) { return (uint64_t)p[0]; }

static inline uint64_t _zhash_wymix(uint64_t A, uint64_t B) 
{
#if defined(__SIZEOF_INT128__)
    unsigned __int128 r = (unsigned __int128)A * B;
    return (uint64_t)r ^ (uint64_t)(r >> 64);
#else
    uint64_t ha = A >> 32, hb = B >> 32, la = (uint32_t)A, lb = (uint32_t)B, hi, lo;
    uint64_t rh = ha * hb, rm0 = ha * lb, rm1 = hb * la, rl = la * lb, t = rl + (rm0 << 32), c = t < rl;
    lo = t + (rm1 << 32); c += lo < t; hi = rh + (rm0 >> 32) + (rm1 >> 32) + c;
    return hi ^ lo;
#endif
}

/* Main hashing function. 
 * Returns a 64-bit hash. 
 */
static inline uint64_t zhash_wyhash(const void *key, size_t len, uint64_t seed) 
{
    const uint8_t *p = (const uint8_t *)key;
    uint64_t see1 = len; seed ^= len;
    static const uint64_t _wyp[] = {
        0xa0761d6478bd642full, 0xe7037ed1a0b428dbull,
        0x8ebc6af09c88c6e3ull, 0x589965cc75374cc3ull
    };

    uint64_t see2 = seed;
    if (len < 16) 
    {
        if (len >= 4) 
        {
            uint64_t a = _zhash_read32(p), b = _zhash_read32(p + len - 4);
            see1 ^= _zhash_wymix(a ^ _wyp[0], see1 ^ _wyp[1]);
            see2 ^= _zhash_wymix(b ^ _wyp[2], see2 ^ _wyp[3]);
        } 
        else if (len > 0) 
        {
            uint64_t a = _zhash_read08(p), b = _zhash_read08(p + len / 2), c = _zhash_read08(p + len - 1);
            see1 ^= _zhash_wymix(a ^ _wyp[0], see1 ^ _wyp[1]);
            see2 ^= _zhash_wymix(b ^ _wyp[2], see2 ^ _wyp[3]);
            see1 ^= _zhash_wymix(c ^ _wyp[0], see1 ^ _wyp[1]); 
        } 
        else 
        {
            see1 ^= _zhash_wymix(_wyp[0], see1 ^ _wyp[1]);
        }
        return _zhash_wymix(see1 ^ see2, _wyp[0]); 
    }
    
    while (len >= 16) 
    {
        uint64_t a = _zhash_read64(p), b = _zhash_read64(p + 8);
        see1 = _zhash_wymix(see1 ^ a, _wyp[0]);
        see2 = _zhash_wymix(see2 ^ b, _wyp[1]);
        p += 16; len -= 16;
    }
    if (len >= 8)
    {
        uint64_t a = _zhash_read64(p);
        see1 = _zhash_wymix(see1 ^ a, _wyp[0]);
        p += 8; len -= 8;
    }
    if (len > 0) 
    {
        uint64_t a = 0;
        if (len >= 4) { a = _zhash_read32(p); p += 4; len -= 4; a <<= 32; }
        if (len >= 2) { a |= (uint64_t)_zhash_read16(p) << (len >= 4 ? 0 : 16); p += 2; len -= 2; }
        if (len >= 1) { a |= _zhash_read08(p); }
        see2 = _zhash_wymix(see2 ^ a, _wyp[1]);
    }
    return _zhash_wymix(see1 ^ see2, _wyp[2]);
}

/* 32-bit wrapper for zmap. */
static inline uint32_t zhash_fast(const void *key, size_t len, uint32_t seed)
{
    uint64_t h = zhash_wyhash(key, len, (uint64_t)seed);
    return (uint32_t)(h ^ (h >> 32));
}

#endif // ZHASH_H
