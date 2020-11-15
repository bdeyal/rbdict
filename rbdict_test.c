#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <ctype.h>

#include "rbdict.h"

const char* word_file = "words.txt";

static int64_t incint(int64_t n)
{
    return n + 1;
}

struct rbdict* build_hash_from_words_str_str()
{
    char word[1000];
    int index = 0;
    char c;

    memset(word, 0, sizeof word);
    struct rbdict* htab = rbdict_create_predefined(RBDICT_STR_STR);

    FILE* fp = fopen(word_file, "r");
    assert(fp);

    while ((c = fgetc(fp)) != EOF) {
        if (c == '\n') {
            word[index] = '\0';
            index = 0;

            if (strlen(word) > 0) {
                /* fprintf(stderr, "%d, Going to set value %s\n", ++wcount, word); */
                if (rbdict_insert_dup(htab, word, word) != 0) {
                    perror("dict insert");
                    exit(1);
                }
            }
        }
        else {
            word[index++] = c;
        }
    }

    fclose(fp);
    return htab;
}

int print_elem(const char* k, const char* v, void* user_data)
{
    printf("%s=%s\n", k, v);
    return 0;
}

int upcase(void* p, void* unused)
{
    char* s = (char*) p;

    while (*s) {
        *s = toupper(*s);
        ++s;
    }

    return 0;
}

void test_rbdict_str_str()
{
    size_t index;
    char** keys;
    size_t dsize;

    struct rbdict* htab = build_hash_from_words_str_str();
    dsize = rbdict_size(htab);

    printf("Data read OK\n");
    printf("Word count = %zu\n", dsize);
    rbdict_foreach(htab, (rbdict_visit_t) print_elem, NULL);

    struct rbdict* clone = rbdict_clone(htab);
    rbdict_destroy(htab);

    /*
     * get the dict keys sorted
     */
    if ((keys = (char**) malloc(dsize * sizeof(char*))) == NULL)
        return;

    if (rbdict_keys(clone, (void**) keys, dsize, 0) != 0)
        return;

    for (index = 0; index < dsize; ++index) {
        char* key = keys[index];
        rbdict_update(clone, key, "", upcase);
        char* val = (char*)rbdict_search(clone, key);
        printf("%s => %s\n", key, val);
    }

    free(keys);
    rbdict_destroy(clone);
}

struct rbdict* build_hash_from_words_str_int()
{
    char word[1000];
    int index = 0;
    char c;

    memset(word, 0, sizeof word);
    struct rbdict* htab = rbdict_create_predefined(RBDICT_STR_INT);

    FILE* fp = fopen(word_file, "r");
    assert(fp);

    while ((c = fgetc(fp)) != EOF) {
        if (c == '\n') {
            word[index] = '\0';
            index = 0;

            if (strlen(word) > 0) {
                //int64_t len = strlen(word);
                if (rbdict_int_update(htab, word, 1, incint) != 0) {
                    perror("dict update");
                    exit(1);
                }

                if (rbdict_int_update(htab, word, 1, incint) != 0) {
                    perror("dict update");
                    exit(1);
                }
            }
        }
        else {
            word[index++] = c;
        }
    }

    rbdict_int_update(htab, "zebroid", 1, incint);

    fclose(fp);
    return htab;
}

int print_elem_int(const char* k, const char* v, void* user_data)
{
    printf("%s=%" PRId64 "\n", k, (int64_t)v);
    return 0;
}

void test_rbdict_str_int()
{
    size_t dsize;

    struct rbdict* htab = build_hash_from_words_str_int();
    dsize = rbdict_size(htab);

    printf("Data read OK\n");
    printf("Word count = %zu\n", dsize);

    struct rbdict* htab2 = rbdict_clone(htab);
    rbdict_foreach(htab2, (rbdict_visit_t) print_elem_int, NULL);
    rbdict_destroy(htab);
    rbdict_destroy(htab2);
}

int print_elem_int_int(const char* k, const char* v, void* user_data)
{
    printf("%" PRId64 "=%" PRId64 "\n", (int64_t)k, (int64_t)v);
    return 0;
}

void test_rbdict_int_int()
{
    size_t dsize;
    int index;

    struct rbdict* htab = rbdict_create_predefined(RBDICT_INT_INT);
    for (index = 0; index <= 100; ++index) {
        int64_t key = index * index;
        int64_t val = index;
        rbdict_insert(htab, key, val);
    }

    dsize = rbdict_size(htab);
    printf("Element count = %zu\n", dsize);
    struct rbdict* htab2 = rbdict_clone(htab);

    rbdict_foreach(htab2, (rbdict_visit_t) print_elem_int_int, NULL);
    rbdict_destroy(htab);
    rbdict_destroy(htab2);
}


int main()
{
    test_rbdict_str_str();
    test_rbdict_str_int();
    test_rbdict_int_int();

    return 0;
}
