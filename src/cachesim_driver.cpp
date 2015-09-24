#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cachesim.hpp"

using namespace std;

void print_help_and_exit(void) {
    printf("cachesim [OPTIONS] < traces/file.trace\n");
    printf("  -c C\t\tTotal size in bytes is 2^C\n");
    printf("  -b B\t\tSize of each block in bytes is 2^B\n");
    printf("  -s S\t\tNumber of blocks per set is 2^S\n");
    printf("  -v V\t\tNumber of blocks in victim cache\n");
    printf("  -k K\t\tPrefetch Distance");
	printf("  -h\t\tThis helpful output\n");
    exit(0);
}

void print_statistics(cache_stats_t* p_stats);

int main(int argc, char* argv[]) {
    int opt;
    uint64_t c = DEFAULT_C;
    uint64_t b = DEFAULT_B;
    uint64_t s = DEFAULT_S;
    uint64_t v = DEFAULT_V;
	uint64_t k = DEFAULT_K;
    FILE* fin  = stdin;

    /* Read arguments */
    while(-1 != (opt = getopt(argc, argv, "c:b:s:i:v:k:h"))) {
        switch(opt) {
        case 'c':
            c = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'v':
            v = atoi(optarg);
            break;
		case 'k':
			k = atoi(optarg);
			break;
        case 'i':
            fin = fopen(optarg, "r");
            break;
        case 'h':
            /* Fall through */
        default:
            print_help_and_exit();
            break;
        }
    }

    cout << "Cache Settings" << endl;
    //cout << "Cache Settings\n");
    cout << "C: "<<c<<endl;
    cout << "B: "<<b<<endl;
    cout << "S: "<<s<<endl;
    cout << "V: "<<v<<endl;
    cout << "K: "<<k<<endl;
    cout<<endl;


    /* Setup the cache */
    setup_cache(c, b, s, v, k);

    /* Setup statistics */
    cache_stats_t stats;
    memset(&stats, 0, sizeof(cache_stats_t));

    /* Begin reading the file */
    char rw;

    uint64_t address;
    while (!feof(fin)) {
        int ret = fscanf(fin, "%c %lx \n", &rw, &address);
        if(ret == 2) {
            cache_access(rw, address, &stats);
        }
    }

    complete_cache(&stats);

    print_statistics(&stats);

    return 0;
}

void print_statistics(cache_stats_t* p_stats) {

    printf("Cache Statistics\n");
    //cout << "Cache Statistics"<<endl;

	//printf("Accesses: %llu\n", p_stats->accesses);
    cout << "Accesses: "<< p_stats->accesses <<endl;

    //printf("Reads: %llu\n", p_stats->reads);
    cout << "Reads: "<< p_stats->reads << endl;

    //printf("Read misses: %llu\n", p_stats->read_misses);
    cout << "Read misses: " << p_stats->read_misses<<endl;

    //printf("Read misses combined: %llu\n", p_stats->read_misses_combined);
    cout << "Read misses combined: "<< p_stats->read_misses_combined << endl;

    //printf("Writes: %llu\n", p_stats->writes);
    cout << "Writes: "<< p_stats->writes<<endl;

    cout << "Write misses: "<< p_stats->write_misses<<endl;
    //printf("Write misses: %llu\n", p_stats->write_misses);

    //printf("Write misses combined: %llu\n", p_stats->write_misses_combined);
    cout << "Write misses combined: "<< p_stats->write_misses_combined<<endl;

    cout << "Misses: "<< p_stats->misses<<endl;
    //printf("Misses: %llu\n", p_stats->misses);

    cout << "Writebacks: " << p_stats->write_backs<< endl;
    //printf("Writebacks: %llu\n", p_stats->write_backs);

    cout << "Victim cache misses: "<< p_stats->vc_misses<<endl;
    //	printf("Victim cache misses: %llu\n", p_stats->vc_misses);

    cout << "Prefetched blocks: "<<p_stats->prefetched_blocks<<endl;
//	printf("Prefetched blocks: %llu\n", p_stats->prefetched_blocks);

    cout << "Useful prefetches: "<< p_stats->useful_prefetches<<endl;
//	printf("Useful prefetches: %llu\n", p_stats->useful_prefetches);

//	printf("Bytes transferred to/from memory: %llu\n", p_stats->bytes_transferred);
    cout << "Bytes transferred to/from memory: "<< p_stats->bytes_transferred<<endl;

//    cout << "Hit Time: " << p_stats->hit_time<< endl;
	printf("Hit Time: %lf\n", p_stats->hit_time);

//    cout << "Miss Penalty: "<< p_stats->miss_penalty<<endl;
    printf("Miss Penalty: %.0f\n", p_stats->miss_penalty);

  //  cout << "Miss rate: " << p_stats->miss_rate << endl;
      printf("Miss rate: %f\n", p_stats->miss_rate);

//    cout << "Average access time (AAT): "<< p_stats->avg_access_time<<endl;

    printf("Average access time (AAT): %f\n", p_stats->avg_access_time);
}

