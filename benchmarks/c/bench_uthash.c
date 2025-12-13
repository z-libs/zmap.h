
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "uthash.h"

struct my_struct 
{
    int id;
    int value;
    UT_hash_handle hh;
};


#define ZMAP_SHORT_NAMES
#define REGISTER_ZMAP_TYPES(X) X(int, int, IntInt)
#include "zmap.h"

#define ITER_ITEMS  1000000
#define ITER_MISSES 1000000

int *keys;
int *values;
int *miss_keys;

/* Helpers. */
static double now() 
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void shuffle(int *array, size_t n) 
{
    if (n > 1) 
    {
        for (size_t i = 0; i < n - 1; i++) 
        {
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

void init_data() 
{
    printf("=> Generating Data (%d items)...\n", ITER_ITEMS);
    keys = (int*)malloc(ITER_ITEMS * sizeof(int));
    values = (int*)malloc(ITER_ITEMS * sizeof(int));
    miss_keys = (int*)malloc(ITER_MISSES * sizeof(int));

    for (int i = 0; i < ITER_ITEMS; i++) 
    {
        keys[i] = i;
        values[i] = i * 2;
    }
    shuffle(keys, ITER_ITEMS);

    for (int i = 0; i < ITER_MISSES; i++) 
    {
        miss_keys[i] = ITER_ITEMS + i;
    }
    shuffle(miss_keys, ITER_MISSES);
}

/* ZMAP wrappers. */
uint32_t hash_int(int k, uint32_t seed) 
{
    return ZMAP_HASH_SCALAR(k, seed);
}

int cmp_int(int a, int b) 
{ 
    return (a == b) ? 0 : (a < b ? -1 : 1);
}

/* Globals for persistence across tests. */
struct my_struct *users = NULL;
map(IntInt) m_zmap;

/* Insertion test. */
double test_insert_uthash() 
{
    double start = now();
    for (int i = 0; i < ITER_ITEMS; i++) 
    {
        struct my_struct *s = (struct my_struct*)malloc(sizeof(struct my_struct));
        s->id = keys[i];
        s->value = values[i];
        HASH_ADD_INT(users, id, s);
    }
    double duration = now() - start;
    printf("[UTHASH Insert] Time: %.4fs\n", duration);
    return duration;
}

double test_insert_zmap() 
{
    m_zmap = map_init(IntInt, hash_int, cmp_int);
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
        struct my_struct *s;
        int k = keys[i];
        HASH_FIND_INT(users, &k, s);
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
        struct my_struct *s;
        int k = miss_keys[i];
        HASH_FIND_INT(users, &k, s);
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
    struct my_struct *current_user, *tmp;
    HASH_ITER(hh, users, current_user, tmp) 
    {
        HASH_DEL(users, current_user);
        free(current_user);
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

    printf("\n=> Benchmark: Insertion (%d items)\n", ITER_ITEMS);
    double t_ut_ins = test_insert_uthash();
    double t_zm_ins = test_insert_zmap();
    if (t_zm_ins < t_ut_ins) printf("   SUMMARY: ZMAP was %.2fx FASTER than UTHASH.\n", t_ut_ins / t_zm_ins);
    else printf("   SUMMARY: ZMAP was %.2fx SLOWER than UTHASH.\n", t_zm_ins / t_ut_ins);

    printf("\n=> Benchmark: Lookup Hit (%d items)\n", ITER_ITEMS);
    double t_ut_hit = test_get_hit_uthash();
    double t_zm_hit = test_get_hit_zmap();
    if (t_zm_hit < t_ut_hit) printf("   SUMMARY: ZMAP was %.2fx FASTER than UTHASH.\n", t_ut_hit / t_zm_hit);
    else printf("   SUMMARY: ZMAP was %.2fx SLOWER than UTHASH.\n", t_zm_hit / t_ut_hit);

    printf("\n=> Benchmark: Lookup Miss (%d items)\n", ITER_MISSES);
    double t_ut_miss = test_get_miss_uthash();
    double t_zm_miss = test_get_miss_zmap();
    if (t_zm_miss < t_ut_miss) printf("   SUMMARY: ZMAP was %.2fx FASTER than UTHASH.\n", t_ut_miss / t_zm_miss);
    else printf("   SUMMARY: ZMAP was %.2fx SLOWER than UTHASH.\n", t_zm_miss / t_ut_miss);

    printf("\n=> Benchmark: Deletion (All items)\n");
    double t_ut_del = test_delete_uthash();
    double t_zm_del = test_delete_zmap();
    if (t_zm_del < t_ut_del) printf("   SUMMARY: ZMAP was %.2fx FASTER than UTHASH.\n", t_ut_del / t_zm_del);
    else printf("   SUMMARY: ZMAP was %.2fx SLOWER than UTHASH.\n", t_zm_del / t_ut_del);

    free(keys); free(values); free(miss_keys);
    return 0;
}
