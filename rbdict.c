#include <string.h>
#include <errno.h>
#include <assert.h>

#include "rbdict.h"
#include "kernel-rbtree.h"

static char* mystrdup(const char* s)
{
    char* res;
    size_t len;

    if (!s) {
        errno = EINVAL;
        return NULL;
    }

    len = strlen(s) + 1;

    res = (char*) malloc(len);
    if (!res) {
        errno = ENOMEM;
        return NULL;
    }

    memcpy(res, s, len);
    return res;
}
/*------------------------------------------------------------*/

struct rbdict {
    struct rb_root root;
    struct rbdict_operations ops;
    size_t nelem;
    int flags;
};
/*----------------------------------------------------------------*/

struct rbdict_pair {
    struct rb_node m_node;
    void* key;
    void* value;
};

inline struct rbdict_pair* node_to_pair(struct rb_node* node)
{
    return rb_entry(node, struct rbdict_pair, m_node);
}
/*----------------------------------------------------------------*/

static struct rbdict_pair* make_rbdict_pair(void* k, void* v)
{
    struct rbdict_pair* n = (struct rbdict_pair*) malloc(sizeof(*n));

    if (!n) {
        errno = ENOMEM;
        return NULL;
    }

    memset(&n->m_node, 0, sizeof(n->m_node));
    n->key = k;
    n->value = v;

    return n;
}
/*----------------------------------------------------------------*/

static void destroy_rbdict_pair(struct rbdict* pDict, struct rbdict_pair* p)
{
    if (p) {
        pDict->ops.k_destroy(p->key);
        pDict->ops.v_destroy(p->value);
        free(p);
    }
}
/*----------------------------------------------------------------*/

static __inline int compare_int(const void* n1, const void* n2)
{
    return (int64_t)n1 - (int64_t)n2;
}
/*----------------------------------------------------------------*/

static __inline void destroy_int(void* n)
{
    return;
}
/*----------------------------------------------------------------*/

static __inline void* clone_int(const void* n)
{
    return (void*)n;
}
/*----------------------------------------------------------------*/

/*
 * create a new empty dictionary
 */
struct rbdict* rbdict_create_ex(const struct rbdict_operations* ops, int flags)
{
    if (ops) {
        if (!(ops->k_compare && ops->k_destroy && ops->k_clone)) {
            errno = EINVAL;
            return NULL;
        }
    }

    struct rbdict* p = (struct rbdict*) malloc(sizeof(struct rbdict));
    if (!p)
        return NULL;

    p->root  = RB_ROOT;
    p->nelem = 0;
    p->flags = flags;

    /*
     *  Init key operations
     */
    if (p->flags & RBDICT_INT_KEY) {
        p->ops.k_compare = &compare_int;
        p->ops.k_destroy = &destroy_int;
        p->ops.k_clone   = &clone_int;
    }
    else if (p->flags & RBDICT_STR_KEY) {
        p->ops.k_compare = (rbdict_compare_t) &strcmp;
        p->ops.k_destroy = (rbdict_destroy_t) &free;
        p->ops.k_clone   = (rbdict_clone_t) &mystrdup;
    }
    else {
        if (!ops)
            goto err_no_ops;
        p->ops.k_compare = ops->k_compare;
        p->ops.k_destroy = ops->k_destroy;
        p->ops.k_clone = ops->k_clone;
    }

    /*
     *  Init value operations
     */
    if (p->flags & RBDICT_INT_VAL) {
        p->ops.v_destroy = &destroy_int;
        p->ops.v_clone   = &clone_int;
    }
    else if (p->flags & RBDICT_STR_VAL) {
        p->ops.v_destroy = (rbdict_destroy_t) &free;
        p->ops.v_clone = (rbdict_clone_t) &mystrdup;
    }
    else {
        if (!ops)
            goto err_no_ops;
        p->ops.v_destroy = ops->v_destroy;
        p->ops.v_clone = ops->v_clone;
    }

    if (!p->ops.v_destroy) {
        p->ops.v_destroy = p->ops.k_destroy;
    }
    if (!p->ops.v_clone) {
        p->ops.v_clone = p->ops.k_clone;
    }
    return p;

err_no_ops:
    free(p);
    errno = EINVAL;
    return NULL;
}
/*----------------------------------------------------------------*/

static void _rbdict_destroy_helper(struct rbdict* pDict, struct rb_node* n)
{
    if (!n)
        return;

    _rbdict_destroy_helper(pDict, n->rb_left);
    _rbdict_destroy_helper(pDict, n->rb_right);

    struct rbdict_pair* e = node_to_pair(n);
    destroy_rbdict_pair(pDict, e);
}
/*----------------------------------------------------------------*/

/*
 * destroy a dictionary
 */
void rbdict_destroy(struct rbdict* pRoot)
{
    struct rb_root* root = &pRoot->root;
    struct rb_node* node = root->rb_node;
    _rbdict_destroy_helper(pRoot, node);
    free(pRoot);
}
/*----------------------------------------------------------------*/

struct rbdict* rbdict_clone(struct rbdict* pSrc)
{
    struct rb_root* src_root = (struct rb_root*) &pSrc->root;
    struct rbdict* pDest;
    struct rb_node* node;

    if (!pSrc) {
        errno = EINVAL;
        return NULL;
    }

    pDest = (struct rbdict*) malloc(sizeof(struct rbdict));
    if (!pDest)
        return NULL;

    pDest->root = RB_ROOT;
    pDest->nelem = 0;
    pDest->ops = pSrc->ops;
    pDest->flags = pSrc->flags;

    for (node = rb_first(src_root);
         node != NULL;
         node = rb_next(node))
    {
        struct rbdict_pair* e = node_to_pair(node);
        if (rbdict_insert_dup(pDest, e->key, e->value) != 0) {
            int err = errno;
            rbdict_destroy(pDest);
            errno = err;
            return NULL;
        }
    }

    return pDest;
}
/*----------------------------------------------------------------*/

/*
 * insert a new key-value pair into an existing dictionary.
 * key and value will be stored as-is
 * ownership of key and value buffers is transfered to dict
 */
int rbdict_insert_nodup(struct rbdict* pRoot, void* key, void* value)
{
    struct rb_root* root = &pRoot->root;
    struct rb_node** new_node = &root->rb_node;
    struct rb_node*  parent = NULL;
    struct rbdict_pair* n = NULL;
    int int_key = (pRoot->flags & RBDICT_INT_KEY);

    /* Figure out where to put new node */
    while (*new_node) {
        struct rbdict_pair* pThis;
        int result;

        parent = *new_node;
        pThis = node_to_pair(parent);

        result = int_key ?
            compare_int(key, pThis->key) :
            pRoot->ops.k_compare(key, pThis->key);

        if (result < 0)
            new_node = &parent->rb_left;
        else if (result > 0)
            new_node = &parent->rb_right;
        else {
            pRoot->ops.v_destroy(pThis->value);
            pThis->value = value;
            return 1;
        }
    }

    /* make new data object with node */
    n = make_rbdict_pair(key, value);

    /* Add new node and rebalance tree. */
    rb_link_node(&n->m_node, parent, new_node);
    rb_insert_color(&n->m_node, root);
    ++pRoot->nelem;

    return 0;
}
/*----------------------------------------------------------------*/

/*
 * insert a new key-value pair into an existing dictionary
 */
int rbdict_insert_dup(struct rbdict* pRoot, void* key, void* value)
{
    void* key_dup;
    void* val_dup;

    if (pRoot->flags & RBDICT_INT_KEY) {
        key_dup = key;
    }
    else {
        if (!key) {
            errno = EINVAL;
            return -1;
        }
        if ((key_dup = pRoot->ops.k_clone(key)) == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }

    if (pRoot->flags & RBDICT_INT_VAL) {
        val_dup = value;
    }
    else {
        if (!value) {
            errno = EINVAL;
            return -1;
        }
        if ((val_dup = pRoot->ops.v_clone(value)) == NULL) {
            pRoot->ops.k_destroy(key_dup);
            errno = ENOMEM;
            return -1;
        }
    }

    if (rbdict_insert_nodup(pRoot, key_dup, val_dup) < 0) {
        pRoot->ops.k_destroy(key_dup);
        pRoot->ops.v_destroy(val_dup);
        errno = ENOMEM;
        return -1;
    }

    return 0;
}
/*----------------------------------------------------------------*/

static struct rbdict_pair* rbdict_search_aux(const struct rbdict* pRoot, const void* key)
{
    struct rb_node* node = pRoot->root.rb_node;
    int int_key = (pRoot->flags & RBDICT_INT_KEY);

    while (node) {
        struct rbdict_pair *pThis = node_to_pair(node);
        int result = int_key ?
            compare_int(key, pThis->key) :
            pRoot->ops.k_compare(key, pThis->key);

        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return pThis;
    }
    return NULL;
}
/*----------------------------------------------------------------*/

/*
 * return the value associated with a key or NULL if no matching
 */
void* rbdict_search(const struct rbdict* pRoot, void* key)
{
    struct rbdict_pair* data = rbdict_search_aux(pRoot, key);
    return data ? data->value : NULL;
}
/*----------------------------------------------------------------*/

/*
 * delete the entry with the given key
 */
void rbdict_delete(struct rbdict* pRoot, const void* key)
{
    struct rbdict_pair* data = rbdict_search_aux(pRoot, key);
    if (data) {
        rb_erase(&data->m_node, &pRoot->root);
        --pRoot->nelem;
        destroy_rbdict_pair(pRoot, data);
    }
}
/*----------------------------------------------------------------*/

/*
 * Number of elements in a dict
 */
size_t rbdict_size(const struct rbdict* pRoot)
{
    return pRoot->nelem;
}
/*----------------------------------------------------------------*/


int rbdict_keys(const struct rbdict* pRoot, void* buf[], size_t bufsize, int flags)
{
    unsigned int bufindex = 0;
    struct rb_root* root = (struct rb_root*) &pRoot->root;
    struct rb_node *node;

    if (bufsize < rbdict_size(pRoot)) {
        errno = EINVAL;
        return -1;
    }

    for (node = rb_first(root); node; node = rb_next(node)) {
        struct rbdict_pair* e = node_to_pair(node);
        buf[bufindex++] = (flags & RBDICT_KEYS_COPY) ? pRoot->ops.k_clone(e->key) : e->key;
    }

    return 0;
}
/*----------------------------------------------------------------*/

void rbdict_foreach(const struct rbdict* pRoot, rbdict_visit_t f, void* user_data)
{
    struct rb_root* root = (struct rb_root*) &pRoot->root;
    struct rb_node *node;

    for (node = rb_first(root); node; node = rb_next(node)) {
        struct rbdict_pair* e = node_to_pair(node);
        f(e->key, e->value, user_data);
    }
}
/*----------------------------------------------------------------*/
