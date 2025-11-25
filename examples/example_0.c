
#include <stdio.h>
#include <string.h>
#include "my_maps.h"

uint32_t hash_str(char *key)
{ 
    return ZMAP_HASH_STR(key); 
}

int cmp_str(char *a, char *b) 
{
    return strcmp(a, b); 
}

uint32_t hash_int(int key) 
{ 
    return ZMAP_HASH_SCALAR(key); 
}

int cmp_int(int a, int b) 
{ 
    return a - b; 
}

int main(void) {
    printf("-> 'String -> Int' Map (Leaderboard)\n");
    
    map_StrInt leaderboard = map_init(StrInt, hash_str, cmp_str);

    printf("  Adding players...\n");
    map_put(&leaderboard, "Alice", 100);
    map_put(&leaderboard, "Bob", 200);
    map_put(&leaderboard, "Charlie", 50);
    map_put(&leaderboard, "Dave", 300);

    printf("  Updating Bob's score (200 -> 250)...\n");
    map_put(&leaderboard, "Bob", 250);

    int *score = map_get(&leaderboard, "Alice");
    if (score) printf("  Alice: %d\n", *score);
    
    score = map_get(&leaderboard, "Bob");
    if (score) printf("  Bob:   %d\n", *score);

    printf("  Removing 'Charlie'...\n");
    map_remove(&leaderboard, "Charlie");

    if (map_get(&leaderboard, "Charlie") == NULL) 
    {
        printf("  Charlie successfully removed.\n");
    }

    printf("  Map size: %zu items\n\n", map_size(&leaderboard));
    
    map_free(&leaderboard);

    printf("-> 'Int -> Float' Map (Product IDs)\n");

    map_IntFloat products = map_init(IntFloat, hash_int, cmp_int);

    map_put(&products, 101, 9.99f);
    map_put(&products, 102, 19.50f);
    map_put(&products, 500, 150.00f);
    map_put(&products, 999, 0.99f);

    printf("  Added 4 products.\n");

    float *price = map_get(&products, 500);
    if (price) printf("  Product 500 Price: $%.2f\n", *price);

    if (map_get(&products, 12345) == NULL) 
    {
        printf("  Product 12345 not found (correct).\n");
    }
    
    printf("  Map size: %zu items\n\n", map_size(&products));

    map_free(&products);

    #ifdef Z_HAS_CLEANUP
    printf("-> Auto-Cleanup Extension\n");
    {
        printf("  Creating auto-free map inside scope...\n");
        map_autofree(StrInt) temp_map = map_init(StrInt, hash_str, cmp_str);
        
        map_put(&temp_map, "Temp1", 1);
        map_put(&temp_map, "Temp2", 2);
        
        printf("  Inside scope: Map has %zu items.\n", map_size(&temp_map));
    }
    printf("  Scope exited. Map freed automatically.\n");
    #endif

    return 0;
}
