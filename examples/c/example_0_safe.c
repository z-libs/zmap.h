
#include <stdio.h>
#include <string.h>
#define ZERROR_IMPLEMENTATION
#define ZERROR_SHORT_NAMES
#include "zerror.h"

#define REGISTER_ZMAP_TYPES(X) \
    X(const char*, int, StrInt)

#define ZMAP_SHORT_NAMES
#include "zmap.h"

uint32_t str_hash(const char* key, uint32_t seed) 
{
    return ZMAP_HASH_STR(key, seed);
}

int str_cmp(const char* a, const char* b) 
{
    return strcmp(a, b);
}

zres process_scores(void)
{
    zmap_autofree(StrInt) scores = map_init(StrInt, str_hash, str_cmp);

    check_ctx(map_put_safe(&scores, "Alice", 100), "Failed to add Alice");
    check_ctx(map_put_safe(&scores, "Bob", 200),   "Failed to add Bob");
    check_ctx(map_put_safe(&scores, "Charlie", 300), "Failed to add Charlie");

    printf("Map size: %zu\n", map_size(&scores));

    int* p_alice = try_into(zres, map_get_safe(&scores, "Alice"));
    printf("Alice's score: %d\n", *p_alice);

    // "Dave" is not in the map. standard map_get would return NULL.
    // map_get_safe returns an Error Result with code Z_ENOTFOUND.
    // We can manually check this without crashing.
    ResPtr_StrInt res_dave = map_get_safe(&scores, "Dave");
    
    if (!res_dave.is_ok) 
    {
        printf("Dave is missing (Expected). Error: %s\n", res_dave.err.msg);
    }

    int* p_bob = unwrap(map_get_safe(&scores, "Bob"));
    printf("Bob's score: %d\n", *p_bob);

    printf("Attempting to access missing key 'Eve'...\n");
    
    int* p_eve = try_into(zres, map_get_safe(&scores, "Eve"));
    
    printf("Eve's score: %d\n", *p_eve);

    return zres_ok();
}

int main(void)
{
    return run(process_scores());
}
