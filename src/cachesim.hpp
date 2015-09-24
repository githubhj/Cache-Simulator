#ifndef CACHESIM_HPP
#define CACHESIM_HPP

#include <cinttypes>
#include <iostream>

//typedef unsigned long long uint64_t;

//My datatypes

// A Block will have data, dirty bit and tag
typedef struct BlockNode {

// tag store
 uint64_t tag;
 // dirty = true then block is dirty
 char dirty;

 // Prefetched Block
 char prefetched;

 // Valid Bit Store
 char valid;

 char dummy;
}BlockNode;

// A doubly linked list is my block
typedef struct BlockLink{
 struct BlockLink *front=NULL, *back=NULL;
 BlockNode link;
}BlockLink;

// A queue of blocks is my set
typedef struct Set{
  BlockLink *begin=NULL;
  BlockLink *end=NULL;
  uint64_t blocks_per_set;
  uint64_t block_size;
  uint64_t b;
  uint64_t s;
  uint64_t filled_blocks;
}Set;

typedef struct Cache{
  uint64_t c, b,s,v,k;
  uint64_t num_sets;
  uint64_t blocks_per_set;
  uint64_t block_size;
  Set **set_array;
}Cache;

typedef struct Prefetcher{
	uint64_t current_miss_addr;
	uint64_t last_miss_addr;
	uint64_t current_stride;
	uint64_t depth ;
	uint64_t last_stride;
	uint64_t first_access;
	bool current_negative_stride;
	bool last_negative_stride;
}Prefetcher;






struct cache_stats_t {
    //writes
	uint64_t accesses;
    //reads
    uint64_t reads;
    //main cache read misses
    uint64_t read_misses;
    //total read misses
    uint64_t read_misses_combined;
    //writes
    uint64_t writes;
    uint64_t write_misses;
    uint64_t write_misses_combined;
    uint64_t misses;
	uint64_t write_backs;
	uint64_t vc_misses;
	uint64_t vc_read_hits;
	uint64_t vc_write_hits;
	uint64_t prefetched_blocks;
	uint64_t useful_prefetches;
	uint64_t bytes_transferred;
	uint64_t evictions;
	double 	 hit_time;
    	double 	 miss_penalty;
    double   miss_rate;
    double   avg_access_time;
};

void cache_access(char rw, uint64_t address, cache_stats_t* p_stats);
void setup_cache(uint64_t c, uint64_t b, uint64_t s, uint64_t v, uint64_t k);
void complete_cache(cache_stats_t *p_stats);


Set* Create_Set(uint64_t,uint64_t);
BlockNode* Create_Victim_Cache(uint64_t);
Set Check_Set(uint64_t);
BlockLink* Check_Block(Set*, uint64_t);
int IfEmptySet(Set*);
BlockLink* Create_BlockLink(uint64_t);
BlockLink* Fetch_Block_In_A_Set(Set*,BlockLink*);
uint64_t GetTag(uint64_t,uint64_t,uint64_t);
uint64_t GetIndex(uint64_t, uint64_t, uint64_t, uint64_t);
uint64_t GetAddress(uint64_t, uint64_t, Cache*);
void Update_Block(Set*, BlockLink*);
void Detach_Block_From_Victim(Set*, BlockLink*);
BlockLink* Prefetch_Block(Set*, BlockLink*);
void Update_Prefetcher(uint64_t, uint64_t, cache_stats_t*);
void InsertABlock(Set*, BlockLink*, BlockLink*, BlockLink*);
void destroy_cache(Cache*);
//End of My datatypes

static const uint64_t DEFAULT_C = 15;   /* 32KB Cache */
static const uint64_t DEFAULT_B = 5;    /* 32-byte blocks */
static const uint64_t DEFAULT_S = 3;    /* 8 blocks per set */
static const uint64_t DEFAULT_V = 4;    /* 4 victim blocks */
static const uint64_t DEFAULT_K = 2;	/* 2 prefetch distance */

/** Argument to cache_access rw. Indicates a load */
static const char     READ = 'r';
/** Argument to cache_access rw. Indicates a store */
static const char     WRITE = 'w';

#endif /* CACHESIM_HPP */
