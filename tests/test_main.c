
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct 
{ 
    float x, y; 
} Vec2;

#define REGISTER_ZMAP_TYPES(X) \
    X(int, int, IntInt)        \
    X(char*, int, StrInt)

#include "zmap.h"

#define TEST(name) printf("[TEST] %-35s", name);
#define PASS() printf(" \033[0;32mPASS\033[0m\n")

uint32_t hash_int(int k, uint32_t seed) { return (uint32_t)k ^ seed; }
int cmp_int(int a, int b) { return a - b; }

uint32_t hash_str(char* k, uint32_t seed) { return ZMAP_HASH_STR(k, seed); }
int cmp_str(char* a, char* b) { return strcmp(a, b); }

void test_basic_ops(void) 
{
    TEST("Init, Put, Get, Size, Free");

    // Init.
    zmap_IntInt m = zmap_init(IntInt, hash_int, cmp_int);
    assert(zmap_size(&m) == 0);

    // Put.
    zmap_put(&m, 10, 100);
    zmap_put(&m, 20, 200);
    zmap_put(&m, 30, 300);

    assert(zmap_size(&m) == 3);

    // Get.
    int* val = zmap_get(&m, 20);
    assert(val != NULL);
    assert(*val == 200);

    val = zmap_get(&m, 99);
    assert(val == NULL);

    // Update existing.
    zmap_put(&m, 20, 299);
    val = zmap_get(&m, 20);
    assert(*val == 299);
    assert(zmap_size(&m) == 3); // Size shouldn't change.

    zmap_free(&m);
    PASS();
}

void test_collisions_and_resize(void) 
{
    TEST("Collisions & Resize (Robin Hood)");

    zmap_IntInt m = zmap_init(IntInt, hash_int, cmp_int);

    // Force many inserts to trigger resize.
    // Default cap is usually 16, so 100 items forces growth.
    for (int i = 0; i < 100; i++) 
    {
        zmap_put(&m, i, i * 10);
    }

    assert(zmap_size(&m) == 100);
    assert(m.capacity >= 128);

    // Verify all data is intact.
    for (int i = 0; i < 100; i++) 
    {
        int* v = zmap_get(&m, i);
        assert(v != NULL);
        assert(*v == i * 10);
    }

    zmap_free(&m);
    PASS();
}

void test_strings(void) 
{
    TEST("String Keys & Removal");

    zmap_StrInt m = zmap_init(StrInt, hash_str, cmp_str);

    zmap_put(&m, "Alice", 1);
    zmap_put(&m, "Bob", 2);
    zmap_put(&m, "Charlie", 3);

    assert(*zmap_get(&m, "Bob") == 2);

    // Remove middle element.
    zmap_remove(&m, "Bob");
    assert(zmap_get(&m, "Bob") == NULL);
    assert(zmap_size(&m) == 2);

    // Ensure others remain (probing chain repair check).
    assert(*zmap_get(&m, "Alice") == 1);
    assert(*zmap_get(&m, "Charlie") == 3);

    zmap_free(&m);
    PASS();
}

void test_iterators(void) 
{
    TEST("Iterators (Foreach)");

    zmap_IntInt m = zmap_init(IntInt, hash_int, cmp_int);
    zmap_put(&m, 1, 10);
    zmap_put(&m, 2, 20);
    zmap_put(&m, 3, 30);

    int sum_k = 0;
    int sum_v = 0;
    int *k_ptr, *v_ptr;

    zmap_foreach(IntInt, &m, k_ptr, v_ptr) 
    {
        sum_k += *k_ptr;
        sum_v += *v_ptr;
    }

    assert(sum_k == 6);  // 1 + 2 + 3
    assert(sum_v == 60); // 10 + 20 + 30

    zmap_free(&m);
    PASS();
}

int main(void) 
{
    printf("=> Running tests (zmap.h, C)\n");
    test_basic_ops();
    test_collisions_and_resize();
    test_strings();
    test_iterators();
    printf("=> All tests passed successfully.\n");
    return 0;
}

