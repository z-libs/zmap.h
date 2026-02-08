/*
 * uthash_compat_demo.c - Demonstrates zmap_uthash.h drop-in compatibility
 *
 * This is standard uthash code that works unchanged with zmap_uthash.h!
 * Compile: gcc -I. examples/uthash_compat_demo.c -o uthash_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The "trojan" */
#include "zmap_uthash.h"
#include <stddef.h>

// Integer keys.

struct my_user {
    int id;             /* key */
    char name[32];
    UT_hash_handle hh;  /* makes this structure hashable */
};

struct my_user *users = NULL;

void add_user(int user_id, const char *name)
{
    struct my_user *s;

    /* Check if already exists */
    HASH_FIND_INT(users, &user_id, s);
    if (s == NULL) {
        s = (struct my_user*)malloc(sizeof(struct my_user));
        s->id = user_id;
        HASH_ADD_INT(users, id, s);
    }
    strncpy(s->name, name, sizeof(s->name) - 1);
    s->name[sizeof(s->name) - 1] = '\0';
}

struct my_user *find_user(int user_id)
{
    struct my_user *s;
    HASH_FIND_INT(users, &user_id, s);
    return s;
}

void delete_user(struct my_user *user)
{
    HASH_DEL(users, user);
    free(user);
}

void delete_all_users(void)
{
    struct my_user *current, *tmp;
    HASH_ITER(hh, users, current, tmp) {
        HASH_DEL(users, current);
        free(current);
    }
}

void print_users(void)
{
    struct my_user *s;
    printf("Users (%u total):\n", HASH_COUNT(users));
    for (s = users; s != NULL; s = (struct my_user*)s->hh.next) {
        printf("  id=%d, name=%s\n", s->id, s->name);
    }
}

// String keys

struct my_item {
    char name[32];      /* key */
    int value;
    UT_hash_handle hh;
};

struct my_item *items = NULL;

void add_item(const char *name, int value)
{
    struct my_item *item;
    
    HASH_FIND_STR(items, name, item);
    if (item == NULL) {
        item = (struct my_item*)malloc(sizeof(struct my_item));
        strncpy(item->name, name, sizeof(item->name) - 1);
        item->name[sizeof(item->name) - 1] = '\0';
        HASH_ADD_STR(items, name, item);
    }
    item->value = value;
}

struct my_item *find_item(const char *name)
{
    struct my_item *item;
    HASH_FIND_STR(items, name, item);
    return item;
}

// Sorting

int name_sort(struct my_user *a, struct my_user *b)
{
    return strcmp(a->name, b->name);
}

int id_sort(struct my_user *a, struct my_user *b)
{
    return (a->id - b->id);
}

// Main

int main(void)
{
    printf("=> zmap_uthash.h Compatibility Demo\n\n");
    printf("Offset tbl: %ld\n", offsetof(UT_hash_handle, tbl));
    printf("Offset prev: %ld\n", offsetof(UT_hash_handle, prev));
    printf("Offset next: %ld\n", offsetof(UT_hash_handle, next));
    printf("Offset hh: %ld\n", offsetof(struct my_user, hh));

    /* Test integer-keyed hash */
    printf("** Integer Keys **\n");
    add_user(42, "Alice");
    add_user(17, "Bob");
    add_user(99, "Charlie");
    add_user(5, "Diana");
    
    print_users();
    


    struct my_user *u = find_user(17);
    if (u) {
        printf("\nFound user 17: %s\n", u->name);
    }
    
    /* Test iteration with HASH_ITER */
    printf("\nIterating with HASH_ITER:\n");
    struct my_user *el, *tmp;
    HASH_ITER(hh, users, el, tmp) {
        printf("  [%d] %s\n", el->id, el->name);
    }
    
    /* Test sorting */


    printf("\nSorted by name:\n");
    HASH_SRT(hh, users, name_sort);
    print_users();
    
    printf("\nSorted by id:\n");
    printf("Before sort: users=%p\n", users);
    HASH_SRT(hh, users, id_sort);
    printf("After sort: users=%p\n", users);
    if (users) printf("After sort: users->next=%p\n", users->hh.next);
    if (users) printf("After sort: users->tbl=%p\n", users->hh.tbl);
    print_users();
    
    /* Test deletion */
    u = find_user(42);
    if (u) {
        printf("\nDeleting user 42...\n");
        delete_user(u);
    } else {
        printf("\nUser 42 not found!\n");
    }
    print_users();
    
    printf("\nCount after deletion: %u\n", HASH_COUNT(users));
    
    /* Cleanup */
    delete_all_users();
    printf("Count after clear: %u\n", HASH_COUNT(users));
    
    /* Test string-keyed hash */
    printf("\n** String Keys **\n");
    add_item("apple", 100);
    add_item("banana", 200);
    add_item("cherry", 300);
    
    struct my_item *it = find_item("banana");
    if (it) {
        printf("Found 'banana': value=%d\n", it->value);
    }
    
    printf("\nAll items (%u total):\n", HASH_COUNT(items));
    struct my_item *item;
    for (item = items; item != NULL; item = (struct my_item*)item->hh.next) {
        printf("  %s = %d\n", item->name, item->value);
    }
    
    /* Cleanup */
    HASH_CLEAR(hh, items);
    printf("\nCleared items, count: %u\n", HASH_COUNT(items));
    
    printf("\n=> All tests passed!\n");
    return 0;
}
