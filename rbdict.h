#ifndef RBDICT_HASH_H
#define RBDICT_HASH_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Foeward decl
 */
struct rbdict;

/*
 *  Function pointer type for comparison, copy, clean and visit update
 */
typedef int     (*rbdict_compare_t)(const void*, const void*);
typedef void    (*rbdict_destroy_t)(void*);
typedef void*   (*rbdict_clone_t)  (const void*);
typedef int     (*rbdict_visit_t)(const void*, const void*, void* user_data);
typedef int     (*rbdict_update_t)(void* value, void* user_data);
typedef int64_t (*rbdict_iupdate_t)(int64_t n);

/*
 *  Operations needed for each dictionary
 */
struct rbdict_operations
{
    /* compare keys */
    rbdict_compare_t k_compare;

    /* key cleanup */
    rbdict_destroy_t k_destroy;

    /* key copy */
    rbdict_clone_t   k_clone;

    /* value cleanup */
    rbdict_destroy_t v_destroy;

    /* value copy */
    rbdict_clone_t   v_clone;
};

/* rbdict creation flags */
enum {
    RBDICT_CUSTOM = 0,
    RBDICT_INT_KEY = 1,
    RBDICT_INT_VAL = (1<<1),
    RBDICT_STR_KEY = (1<<2),
    RBDICT_STR_VAL = (1<<3),
    RBDICT_STR_STR = (RBDICT_STR_KEY | RBDICT_STR_VAL),
    RBDICT_STR_INT = (RBDICT_STR_KEY | RBDICT_INT_VAL),
    RBDICT_INT_STR = (RBDICT_INT_KEY | RBDICT_STR_VAL),
    RBDICT_INT_INT = (RBDICT_INT_KEY | RBDICT_INT_VAL)
};

/*
 * create a new empty dictionary. Must provide OPS functions structure
 * in rbdict_operations structure for comparison cleanup and duplication
 * respectively.
 */
struct rbdict* rbdict_create_ex(const struct rbdict_operations* ops, int flags);

inline struct rbdict* rbdict_create(const struct rbdict_operations* ops)
{
    return rbdict_create_ex(ops, RBDICT_CUSTOM);
}

inline struct rbdict* rbdict_create_predefined(int flags)
{
    return rbdict_create_ex(NULL, flags);
}

/*
 * destroy a dictionary
 */
void rbdict_destroy(struct rbdict*);

/*
 *  Deep copy
 */
struct rbdict* rbdict_clone(struct rbdict*);

/*
 * insert a new key-value pair into an existing dictionary
 * A clone of key and value will be stored in the dict
 */
int rbdict_insert_dup(struct rbdict*, void* key, void* value);

/*
 * insert a new key-value pair into an existing dictionary.
 * Ownership of key and value buffers is transfered to dict
 */
int rbdict_insert_nodup(struct rbdict*, void* key, void* value);

/*
 * Syntactic sugar. insert (nodup) with automatic cast
 */
#define rbdict_insert(dict, k, v) \
    rbdict_insert_nodup((dict), ((void*)(uintptr_t)(k)), ((void*)(uintptr_t)(v)))

/*
 * Update an existing key calling an updater function.
 * if does not exist create with default_value.
 * Always duplicate key and value
 */
int rbdict_update_ex(struct rbdict*,
                     const void* key,
                     const void* default_value,
                     rbdict_update_t updater,
                     void* user_data);

/*
 * Syntactic sugar. update with automatic cast and NULL user data
 */
#define rbdict_update(dict, k, v, u)    \
    rbdict_update_ex((dict), ((void*)(uintptr_t)(k)), ((void*)(uintptr_t)(v)), (u), NULL)

/*
 * increment value (in case of a integral valued rbdict)
 */
int rbdict_int_update(struct rbdict* pRoot, const void* key, int64_t default_value, rbdict_iupdate_t f);

/*
 * Return the value associated with a key or NULL if no match
 */
void* rbdict_search(const struct rbdict* pRoot, void* key);

/*
 * delete the entry with the given key
 */
void rbdict_delete(struct rbdict* pRoot, const void *key);

/*
 * Number of elements in a dict
 */
size_t rbdict_size(const struct rbdict* pRoot);

/*
 *  Fill in the keys in the supplied buffer.
 */
enum {
    RBDICT_KEYS_SORTED = 1,
    RBDICT_KEYS_CLONE = 2,
    RBDICT_VALUES_CLONE = 4,
};

int rbdict_keys(const struct rbdict* pRoot, void* buf[], size_t bufsize, int flags);
int rbdict_values(const struct rbdict* pRoot, void* buf[], size_t bufsize, int flags);

/*
 * foreach calls the provided function for each pair in the dict
 */
void rbdict_foreach(const struct rbdict* pRoot, rbdict_visit_t f, void* user_data);

#ifdef __cplusplus
}
#endif

#endif
