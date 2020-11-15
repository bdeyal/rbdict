#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <ctype.h>
#include "rbdict.h"

static int64_t incint(int64_t n) { return n + 1; }

struct rbdict* build_word_count_dict(const char* word_file)
{
    char word[1000];
    int index = 0;
    char c;

    memset(word, 0, sizeof word);
    struct rbdict* htab = rbdict_create_predefined(RBDICT_STR_INT);

    FILE* fp = fopen(word_file, "r");
    assert(fp);

    while ((c = fgetc(fp)) != EOF) {
        if (isalpha(c)) {
            word[index++] = tolower(c);
        }
        else if (index) {
            word[index] = '\0';
            index = 0;

            if (rbdict_int_update(htab, word, 1, incint) != 0) {
                perror("dict update");
                exit(1);
            }
        }
    }

    fclose(fp);
    return htab;
}

int print_elem_int(const void* k, const void* v, void* user_data)
{
    int64_t print_threshold =
        user_data ?
        *((int64_t*)user_data) :
        0;

    if ((int64_t)v > print_threshold) {
        printf("%s=%" PRId64 "\n", (const char*)k, (int64_t)v);
    }
    return 0;
}

void test_rbdict_str_int(const char* fname, int64_t print_threshold)
{
    size_t dsize;

    struct rbdict* htab = build_word_count_dict(fname);
    dsize = rbdict_size(htab);

    printf("Data read OK\n");
    printf("Word count = %zu\n", dsize);

    struct rbdict* htab2 = rbdict_clone(htab);
    rbdict_foreach(htab2, print_elem_int, &print_threshold);
    rbdict_destroy(htab);
    rbdict_destroy(htab2);
}

int main(int argc, char* argv[])
{
    int print_threshold = 0;
    const char* fname = 0;

    if (argc == 1) {
        printf("Usage: %s <text file>\n", argv[0]);
        return 0;
    }

    fname = argv[1];

    if (argc > 2) {
        print_threshold = atoi(argv[2]);
    }

    test_rbdict_str_int(fname, print_threshold);
    return 0;
}
