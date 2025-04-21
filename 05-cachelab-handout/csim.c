#include "cachelab.h"
#include "getopt.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct cache_line
{
    uint8_t  valid;
    uint64_t tag;
    uint32_t time_stamp;
} cache_line_t;

typedef struct cache_set
{
    cache_line_t* lines;
} cache_set_t;

typedef struct cache
{
    cache_set_t* sets;
} cache_t;

cache_t* cache;

static uint8_t s = 0, E = 0, b = 0;
static bool    verbose = false;
static char*   file;

static uint8_t S = 0;

static uint32_t current_time = 0;

static uint32_t hit_count = 0, miss_count = 0, eviction_count = 0;

void usage()
{
    printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <s>     Number of s-bitset index.\n");
    printf("  -E <E>     Number of E-way associative.\n");
    printf("  -b <b>     Number of b-bitset offset.\n");
    printf("  -t <tracefile>  Tracefile used for cache simulation.\n");
}
void parse_options(int argc, char* argv[])
{
    char opt;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
        case 'h': usage(); exit(0);
        case 'v': verbose = true; break;
        case 's':
            s = atoi(optarg);
            S = 1 << s;
            break;
        case 'E': E = atoi(optarg); break;
        case 'b': b = atoi(optarg); break;
        case 't': file = optarg; break;
        default:
            printf("Invalid option: %c\n", opt);
            usage();
            exit(1);
        }
    }

    if (verbose)
        printf("[input] s = %d, S = %d, E = %d, b = %d, file = %s, verbose = %d\n",
               s,
               S,
               E,
               b,
               file,
               verbose);
}

void init_cache()
{
    cache       = (cache_t*)malloc(sizeof(cache_t));
    cache->sets = (cache_set_t*)malloc(sizeof(cache_set_t) * S);
    for (uint8_t i = 0; i < S; i++) {
        cache->sets[i].lines = (cache_line_t*)malloc(sizeof(cache_line_t) * E);
        memset(cache->sets[i].lines, 0, sizeof(cache_line_t) * E);
    }
}

void free_cache()
{
    for (uint8_t i = 0; i < S; i++) {
        free(cache->sets[i].lines);
    }
    free(cache->sets);
    free(cache);
}

void update_cache_line(uint64_t tag, uint64_t set_index, uint32_t line_index)
{
    cache->sets[set_index].lines[line_index].tag        = tag;
    cache->sets[set_index].lines[line_index].valid      = 1;
    cache->sets[set_index].lines[line_index].time_stamp = current_time;
}
void find_cache_line(uint64_t tag, uint64_t set_index, char* cache_result)
{
    int          empty_line = -1;
    cache_set_t* set        = cache->sets + set_index;

    current_time++;

    for (uint32_t i = 0; i < E; i++) {
        if (set->lines[i].valid == 0) {
            empty_line = i;
        }
        else if (set->lines[i].tag == tag) {
            strcat(cache_result, "hit ");
            hit_count++;
            update_cache_line(tag, set_index, i);
            return;
        }
    }

    strcat(cache_result, "miss ");
    miss_count++;

    if (empty_line == -1) {
        strcat(cache_result, "eviction ");
        // empty line not existed
        // all cache line are valid, but no hit. We need to evict one line by LRU.

        eviction_count++;
        uint32_t evict_line = 0;
        uint32_t min_time   = INT32_MAX;
        for (uint32_t i = 1; i < E; i++) {
            if (min_time > set->lines[i].time_stamp) {
                evict_line = i;
                min_time   = set->lines[i].time_stamp;
            }
        }

        update_cache_line(tag, set_index, evict_line);
    }
    else {
        update_cache_line(tag, set_index, empty_line);
    }
}

void parse_trace_file()
{

    uint64_t addr;
    char     op;
    uint32_t size;

    FILE* fp = fopen(file, "r");
    char  cache_result[50];

    while (fscanf(fp, " %c %lx, %d", &op, &addr, &size) > 0) {

        if (op == 'I') {
            continue;
        }

        memset(cache_result, 0, sizeof(cache_result));

        uint64_t tag       = addr >> (b + s);
        uint64_t set_index = (addr >> b) & ((1 << s) - 1);

        find_cache_line(tag, set_index, cache_result);
        if (op == 'M') {
            find_cache_line(tag, set_index, cache_result);
        }

        if (verbose) printf("%c %lx,%d %s\n", op, addr, size, cache_result);
    }

    fclose(fp);
}


int main(int argc, char* argv[])
{
    parse_options(argc, argv);
    init_cache();
    parse_trace_file();
    printSummary(hit_count, miss_count, eviction_count);
    free_cache();

    return 0;
}
