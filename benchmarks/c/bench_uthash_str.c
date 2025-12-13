
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "uthash.h"

struct str_node 
{
    char key[64];
    int value;
    UT_hash_handle hh;
};

#define ZMAP_SHORT_NAMES
#define REGISTER_ZMAP_TYPES(X) X(char*, int, StrMap)
#include "zmap.h"

#define ITER_ITEMS  1000000
#define ITER_MISSES 1000000

/* Global Data */
char (*keys)[64];
int *values;
char (*miss_keys)[64];

/* Helpers. */
static double now() 
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void generate_strings(char (*arr)[64], size_t n) 
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < n; i++) 
    {
        int len = 16 + (rand() % 47); /* Length 16-63 */
        for (int c = 0; c < len; c++) 
        {
            arr[i][c] = charset[rand() % (sizeof(charset) - 1)];
        }
        arr[i][len] = '\0';
    }
}

void init_data() 
{
    printf("=> Generating Data (%d strings)...\n", ITER_ITEMS);
    keys = (char (*)[64])malloc(ITER_ITEMS * 64);
    miss_keys = (char (*)[64])malloc(ITER_MISSES * 64);
    values = (int*)malloc(ITER_ITEMS * sizeof(int));

    generate_strings(keys, ITER_ITEMS);
    generate_strings(miss_keys, ITER_MISSES);
    for (int i = 0; i < ITER_ITEMS; i++) values[i] = i;
}

/* ZMAP Wrappers. */
uint32_t hash_str(char* k, uint32_t seed) 
{ 
    return ZMAP_HASH_STR(k, seed); 
}

int cmp_str(char* a, char* b) 
{ 
    return strcmp(a, b); 
}

/* Globals for persistence across tests. */
struct str_node *users = NULL;
map(StrMap) m_zmap;

/* Insertion test. */
double test_insert_uthash() 
{
    double start = now();
    for (int i = 0; i < ITER_ITEMS; i++) 
    {
        struct str_node *s = (struct str_node*)malloc(sizeof(struct str_node));
        strcpy(s->key, keys[i]);
        s->value = values[i];
        HASH_ADD_STR(users, key, s);
    }
    double duration = now() - start;
    printf("[UTHASH Insert] Time: %.4fs\n", duration);
    return duration;
}

double test_insert_zmap() 
{
    m_zmap = map_init(StrMap, hash_str, cmp_str);
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
        struct str_node *s;
        HASH_FIND_STR(users, keys[i], s);
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
        struct str_node *s;
        HASH_FIND_STR(users, miss_keys[i], s);
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
    struct str_node *current_user, *tmp;
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

    printf("\n=> Benchmark String: Insertion (%d items)\n", ITER_ITEMS);
    double t_ut_ins = test_insert_uthash();
    double t_zm_ins = test_insert_zmap();
    if (t_zm_ins < t_ut_ins) printf("   SUMMARY: ZMAP was %.2fx FASTER than UTHASH.\n", t_ut_ins / t_zm_ins);
    else printf("   SUMMARY: ZMAP was %.2fx SLOWER than UTHASH.\n", t_zm_ins / t_ut_ins);

    printf("\n=> Benchmark String: Lookup Hit (%d items)\n", ITER_ITEMS);
    double t_ut_hit = test_get_hit_uthash();
    double t_zm_hit = test_get_hit_zmap();
    if (t_zm_hit < t_ut_hit) printf("   SUMMARY: ZMAP was %.2fx FASTER than UTHASH.\n", t_ut_hit / t_zm_hit);
    else printf("   SUMMARY: ZMAP was %.2fx SLOWER than UTHASH.\n", t_zm_hit / t_ut_hit);

    printf("\n=> Benchmark String: Lookup Miss (%d items)\n", ITER_MISSES);
    double t_ut_miss = test_get_miss_uthash();
    double t_zm_miss = test_get_miss_zmap();
    if (t_zm_miss < t_ut_miss) printf("   SUMMARY: ZMAP was %.2fx FASTER than UTHASH.\n", t_ut_miss / t_zm_miss);
    else printf("   SUMMARY: ZMAP was %.2fx SLOWER than UTHASH.\n", t_zm_miss / t_ut_miss);

    printf("\n=> Benchmark String: Deletion (All items)\n");
    double t_ut_del = test_delete_uthash();
    double t_zm_del = test_delete_zmap();
    if (t_zm_del < t_ut_del) printf("   SUMMARY: ZMAP was %.2fx FASTER than UTHASH.\n", t_ut_del / t_zm_del);
    else printf("   SUMMARY: ZMAP was %.2fx SLOWER than UTHASH.\n", t_zm_del / t_ut_del);

    free(keys); free(miss_keys); free(values);
    return 0;
}
