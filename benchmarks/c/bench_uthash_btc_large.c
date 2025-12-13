#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "uthash.h"

/* Bitcoin type. */
typedef struct { uint8_t data[32]; } uint256;

struct btc_node 
{
    uint256 txid;
    int value;
    UT_hash_handle hh; 
};

#define ZMAP_SHORT_NAMES
#define REGISTER_MAP_TYPES(X) X(uint256, int, BtcMap)
#include "zmap.h"

/* Increased to 10 Million items.
 * Total RAM usage approx: 2GB (Aux arrays + Hash Map overhead).
 * This forces RAM thrashing by overflowing L3 Cache (~32MB).
 */
#define ITER_ITEMS  10000000
#define ITER_MISSES 10000000

uint256 *keys;
int *values;
uint256 *miss_keys;

/* Helpers. */
static double now() 
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void generate_keys(uint256 *arr, size_t n) 
{
    for (size_t i = 0; i < n; i++) 
    {
        for (int b = 0; b < 32; b++) 
        {
            arr[i].data[b] = rand() % 256;
        }
    }
}

void init_data() 
{
    printf("=> Generating Large Dataset (TxIDs: %d)...\n", ITER_ITEMS);
    printf("   (This may take several seconds...)\n");
    
    keys = (uint256*)malloc(ITER_ITEMS * sizeof(uint256));
    values = (int*)malloc(ITER_ITEMS * sizeof(int));
    miss_keys = (uint256*)malloc(ITER_MISSES * sizeof(uint256));

    if (!keys || !values || !miss_keys) {
        fprintf(stderr, "ERROR: Failed to allocate memory for dataset. You need ~700MB free for raw data.\n");
        exit(1);
    }

    generate_keys(keys, ITER_ITEMS);
    generate_keys(miss_keys, ITER_MISSES);
    for (int i = 0; i < ITER_ITEMS; i++) values[i] = i;
}

/* ZMAP wrappers. */
uint32_t hash_btc(uint256 k, uint32_t seed) 
{
    uint32_t h; 
    memcpy(&h, k.data, sizeof(uint32_t)); 
    return h; 
}

int cmp_btc(uint256 a, uint256 b) 
{
    return memcmp(a.data, b.data, 32);
}

struct btc_node *mempool_ut = NULL;
map(BtcMap) m_zmap;

/* Insertion test. */
double test_insert_uthash() 
{
    double start = now();
    for (int i = 0; i < ITER_ITEMS; i++) 
    {
        struct btc_node *s = (struct btc_node*)malloc(sizeof(struct btc_node));
        if (!s) { fprintf(stderr, "OOM in UTHASH insert\n"); exit(1); }
        s->txid = keys[i];
        s->value = values[i];
        HASH_ADD(hh, mempool_ut, txid, sizeof(uint256), s);
    }
    double duration = now() - start;
    printf("[UTHASH Insert] Time: %.4fs\n", duration);
    return duration;
}

double test_insert_zmap() 
{
    m_zmap = map_init(BtcMap, hash_btc, cmp_btc);
    double start = now();
    for (int i = 0; i < ITER_ITEMS; i++) 
    {
        map_put(&m_zmap, keys[i], values[i]);
    }
    double duration = now() - start;
    printf("[ZMAP Insert]   Time: %.4fs\n", duration);
    return duration;
}

/* Get (Hit) test. */
double test_get_hit_uthash() 
{
    double start = now();
    volatile int found = 0;
    for (int i = 0; i < ITER_ITEMS; i++) 
    {
        struct btc_node *s;
        HASH_FIND(hh, mempool_ut, &keys[i], sizeof(uint256), s);
        if (s) found++;
    }
    double duration = now() - start;
    printf("[UTHASH Hit]    Time: %.4fs (Found: %d)\n", duration, found);
    return duration;
}

double test_get_hit_zmap() 
{
    double start = now();
    volatile int found = 0;
    for (int i = 0; i < ITER_ITEMS; i++) 
    {
        int *val = map_get(&m_zmap, keys[i]);
        if (val) found++;
    }
    double duration = now() - start;
    printf("[ZMAP Hit]      Time: %.4fs (Found: %d)\n", duration, found);
    return duration;
}

/* Get (Miss) test. */
double test_get_miss_uthash() 
{
    double start = now();
    volatile int found = 0;
    for (int i = 0; i < ITER_MISSES; i++) 
    {
        struct btc_node *s;
        HASH_FIND(hh, mempool_ut, &miss_keys[i], sizeof(uint256), s);
        if (s) found++;
    }
    double duration = now() - start;
    printf("[UTHASH Miss]   Time: %.4fs (Found: %d)\n", duration, found);
    return duration;
}

double test_get_miss_zmap() 
{
    double start = now();
    volatile int found = 0;
    for (int i = 0; i < ITER_MISSES; i++) 
    {
        int *val = map_get(&m_zmap, miss_keys[i]);
        if (val) found++;
    }
    double duration = now() - start;
    printf("[ZMAP Miss]     Time: %.4fs (Found: %d)\n", duration, found);
    return duration;
}

/* Delete test. */
double test_delete_uthash() 
{
    double start = now();
    struct btc_node *current, *tmp;
    HASH_ITER(hh, mempool_ut, current, tmp) 
    {
        HASH_DEL(mempool_ut, current);
        free(current);
    }
    double duration = now() - start;
    printf("[UTHASH Delete] Time: %.4fs\n", duration);
    return duration;
}

double test_delete_zmap() 
{
    double start = now();
    map_free(&m_zmap);
    double duration = now() - start;
    printf("[ZMAP Delete]   Time: %.4fs\n", duration);
    return duration;
}

int main(void) 
{
    srand(time(NULL));
    init_data();

    printf("\n=> Benchmark BTC: Insertion (%d items)\n", ITER_ITEMS);
    double t_ut_ins = test_insert_uthash();
    double t_zm_ins = test_insert_zmap();
    if (t_zm_ins < t_ut_ins) printf("   SUMMARY: ZMAP was %.2fx FASTER than UTHASH.\n", t_ut_ins / t_zm_ins);
    else printf("   SUMMARY: ZMAP was %.2fx SLOWER than UTHASH.\n", t_zm_ins / t_ut_ins);

    printf("\n=> Benchmark BTC: Lookup Hit (%d items)\n", ITER_ITEMS);
    double t_ut_hit = test_get_hit_uthash();
    double t_zm_hit = test_get_hit_zmap();
    if (t_zm_hit < t_ut_hit) printf("   SUMMARY: ZMAP was %.2fx FASTER than UTHASH.\n", t_ut_hit / t_zm_hit);
    else printf("   SUMMARY: ZMAP was %.2fx SLOWER than UTHASH.\n", t_zm_hit / t_ut_hit);

    printf("\n=> Benchmark BTC: Lookup Miss (%d items)\n", ITER_MISSES);
    double t_ut_miss = test_get_miss_uthash();
    double t_zm_miss = test_get_miss_zmap();
    if (t_zm_miss < t_ut_miss) printf("   SUMMARY: ZMAP was %.2fx FASTER than UTHASH.\n", t_ut_miss / t_zm_miss);
    else printf("   SUMMARY: ZMAP was %.2fx SLOWER than UTHASH.\n", t_zm_miss / t_ut_miss);

    printf("\n=> Benchmark BTC: Deletion (All items)\n");
    double t_ut_del = test_delete_uthash();
    double t_zm_del = test_delete_zmap();
    if (t_zm_del < t_ut_del) printf("   SUMMARY: ZMAP was %.2fx FASTER than UTHASH.\n", t_ut_del / t_zm_del);
    else printf("   SUMMARY: ZMAP was %.2fx SLOWER than UTHASH.\n", t_zm_del / t_ut_del);

    free(keys); 
    free(values); 
    free(miss_keys);
    return 0;
}
