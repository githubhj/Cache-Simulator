#include "cachesim.hpp"
#include <cmath>
#include <deque>
#include <stdlib.h>
#include <iostream>

using namespace std;

Cache* Cache_Ptr;
Cache* Victim_Cache_Ptr;
Prefetcher* Prefetcher_Ptr;

/**
 * Subroutine for initializing the cache. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @c The total number of bytes for data storage is 2^C
 * @b The size of a single cache line in bytes is 2^B
 * @s The number of blocks in each set is 2^S
 * @v The number of blocks in the victim cache is 2^V
 * @k The prefetch distance is K
 */
void setup_cache(uint64_t c, uint64_t b, uint64_t s, uint64_t v, uint64_t k) {
	const uint64_t cache_size = pow(2,c);
	const uint64_t block_size = pow(2,b);
	const uint64_t blocks_per_set = pow(2,s);
	const uint64_t blocks_victim_cache = v;
	//Block size*Block per set*No. of sets = Cache size
	const uint64_t temp_var = ((c-s)-b);
	const uint64_t set_num = pow(2,temp_var);


	//Create Main Cache, an array of sets
	Cache_Ptr = (Cache*)malloc(sizeof(Cache));
	//assign c,b,s,v
	Cache_Ptr->c = c;
	Cache_Ptr->b = b;
	Cache_Ptr->s = s;
	Cache_Ptr->v = v;
	Cache_Ptr->k = k;
	Cache_Ptr->block_size = block_size;
	//Number of sets in this cache
	Cache_Ptr->num_sets= set_num;
	//Create Set array
	Cache_Ptr->set_array = (Set**) malloc(set_num*sizeof(Set*));
	//Create Each Set
	for(uint64_t i=0; i<set_num; i++){
		Cache_Ptr->set_array[i] = Create_Set(s, b);
	}

	//Create victim Cache if required
	if(v!=0){
		Victim_Cache_Ptr = (Cache*)malloc(sizeof(Cache));
		//assign c,b,s
		Victim_Cache_Ptr->c = (uint64_t)log2(v*block_size);
		Victim_Cache_Ptr->b = b;
		Victim_Cache_Ptr->s = (uint64_t)log2(v);
		Victim_Cache_Ptr->v = 0;
		Victim_Cache_Ptr->k = 0;
		Victim_Cache_Ptr->blocks_per_set = v;
		//should be 0 in our case, Fully Associative
		uint64_t temp_num_sets = ((Victim_Cache_Ptr->c - Victim_Cache_Ptr->s)-Victim_Cache_Ptr->b);
		uint64_t victim_num_sets = pow(2,temp_num_sets);
		Victim_Cache_Ptr->num_sets = victim_num_sets;

		Victim_Cache_Ptr->set_array = (Set**)malloc(victim_num_sets*sizeof(Set*));
		//Create each set, in our case just one
		for(uint64_t i=0; i<victim_num_sets; i++){
				Victim_Cache_Ptr->set_array[i] = Create_Set(Victim_Cache_Ptr->s, Victim_Cache_Ptr->b);
		}
	}

	//Create Prefetcher if required
	if(k!=0){
		Prefetcher_Ptr = (Prefetcher*)malloc(sizeof(Prefetcher));
		Prefetcher_Ptr->depth = k;
		Prefetcher_Ptr->current_miss_addr = 0;
		Prefetcher_Ptr->current_stride = 0;
		Prefetcher_Ptr->first_access = 1;
		Prefetcher_Ptr->last_miss_addr = 0;
		Prefetcher_Ptr->last_stride = 0;
		Prefetcher_Ptr->current_negative_stride = false;
		Prefetcher_Ptr->last_negative_stride = false;
	}
}
/**
 * Subroutine that simulates the cache one trace event at a time.
 * XXX: You're responsible for completing this routine
 *
 * @rw The type of event. Either READ or WRITE
 * @address  The target memory address
 * @p_stats Pointer to the statistics structure
 */
void cache_access(char rw, uint64_t address, cache_stats_t* p_stats) {

	//Update p_stats
	p_stats->accesses++;
	if(rw == READ)
		p_stats->reads++;
	else
		p_stats->writes++;

	//Get Tag & Index value from address
	uint64_t main_tag = GetTag(address, Cache_Ptr->c, Cache_Ptr->s);
	uint64_t main_index = GetIndex(address, Cache_Ptr->c, Cache_Ptr->b, Cache_Ptr->s);

	//Get desired set
	Set* main_cache_set_ptr = Cache_Ptr->set_array[main_index];

	//Check if there is a block in main cache
	BlockLink* main_cache_block_ptr = Check_Block(main_cache_set_ptr,main_tag);

	//If Block not found in Main Cache
	if(main_cache_block_ptr == NULL){

		//Increment Misses
		p_stats->misses++;
		if(Cache_Ptr->v==0){
		p_stats->vc_misses++;
		}

		if(rw == READ){
			p_stats->read_misses++;
			if(Cache_Ptr->v==0){
				p_stats->read_misses_combined++;
			}
		}
		else{
			p_stats->write_misses++;
			if(Cache_Ptr->v==0){
				p_stats->write_misses_combined++;
			}
		}

		//Victim Cache Present
		if(Cache_Ptr->v!=0){

			//Get Tag & Index value from address
			uint64_t victim_tag = GetTag(address, Victim_Cache_Ptr->c, Victim_Cache_Ptr->s);
			uint64_t victim_index = GetIndex(address, Victim_Cache_Ptr->c, Victim_Cache_Ptr->b, Victim_Cache_Ptr->s);

			//Get desired set
			Set* victim_cache_set_ptr = Victim_Cache_Ptr->set_array[victim_index];

			//Check if there is a block in main cache
			BlockLink* victim_cache_block_ptr = Check_Block(victim_cache_set_ptr,victim_tag);

			//If not found in victim Cache
			if(victim_cache_block_ptr == NULL){

				//Increment Victim Cache Misses
				p_stats->vc_misses++;
				if(rw ==READ){
					p_stats->read_misses_combined++;
				}
				else{
					p_stats->write_misses_combined++;
				}

				//Create a New BlockLink
				BlockLink* new_block_ptr = Create_BlockLink(main_tag);

				//If its a write make this block dirty
				//Check if write
				if(rw == WRITE){
				//make the block dirty
					new_block_ptr->link.dirty = 1;
				}

				//Fetch this Block in Main Cache
				BlockLink* evicted_main_block_ptr = Fetch_Block_In_A_Set(main_cache_set_ptr,new_block_ptr);

				//If an old block is evicted try fitting it in victim
				if(evicted_main_block_ptr!=NULL){

					//Get evicted address from Main Cache
					uint64_t evicted_address = GetAddress(evicted_main_block_ptr->link.tag, main_index, Cache_Ptr);

					//Get Tag & Index value from address
					uint64_t evicted_victim_tag = GetTag(evicted_address, Victim_Cache_Ptr->c, Victim_Cache_Ptr->s);
					uint64_t evicted_victim_index = GetIndex(evicted_address, Victim_Cache_Ptr->c, Victim_Cache_Ptr->b, Victim_Cache_Ptr->s);

					//Get desired set
					Set* evicted_victim_cache_set_ptr = Victim_Cache_Ptr->set_array[evicted_victim_index];

					//Change tag as per victim cache
					evicted_main_block_ptr->link.tag = evicted_victim_tag;

					//Try fitting block in victim cache
					BlockLink* evicted_victim_ptr = Fetch_Block_In_A_Set(evicted_victim_cache_set_ptr, evicted_main_block_ptr);

					//If something is evicted delete it
					if(evicted_victim_ptr!=NULL){
						if(evicted_victim_ptr->link.dirty==1){
							p_stats->write_backs++;
						}
						p_stats->evictions++;
						free(evicted_victim_ptr);
					}
				}
			}

			// Block found in Victim Cache
			else{

				//Increment VC hits
				if(rw==READ){
					p_stats->vc_read_hits++;
				}
				else{
					p_stats->vc_write_hits++;
				}
				//Check if write
				if(rw == WRITE){
					//make the block dirty
					victim_cache_block_ptr->link.dirty = 1;
				}

				//Check if prefetched
				if((victim_cache_block_ptr->link.prefetched==1)){
					//Increment useful prefetch
					p_stats->useful_prefetches++;
					victim_cache_block_ptr->link.prefetched =2;
				}

				//Latch victim ptr front and back
				BlockLink* victim_block_ptr_front = victim_cache_block_ptr->front;
				BlockLink* victim_block_ptr_back = victim_cache_block_ptr->back;

				//Block found in victim,So put it in Main Cache
				//Step1: Case 1 Remove from victim set
				Detach_Block_From_Victim(victim_cache_set_ptr,victim_cache_block_ptr);

				//Step2: Change tag to main cache tag
				victim_cache_block_ptr->link.tag = main_tag;

				//Step3: Fetch in Main Cache
				BlockLink* evicted_main_block_ptr = Fetch_Block_In_A_Set(main_cache_set_ptr, victim_cache_block_ptr);

				//Step4: If anything else evicted then put in victim
				if(evicted_main_block_ptr!=NULL){

					//Get evicted address from Main Cache
					uint64_t evicted_address = GetAddress(evicted_main_block_ptr->link.tag, main_index, Cache_Ptr);

					//Get Tag & Index value from address
					uint64_t evicted_victim_tag = GetTag(evicted_address, Victim_Cache_Ptr->c, Victim_Cache_Ptr->s);
					uint64_t evicted_victim_index = GetIndex(evicted_address, Victim_Cache_Ptr->c, Victim_Cache_Ptr->b, Victim_Cache_Ptr->s);
					//Get desired set
					Set* evicted_victim_cache_set_ptr = Victim_Cache_Ptr->set_array[evicted_victim_index];

					//Change tag as per victim cache
					evicted_main_block_ptr->link.tag = evicted_victim_tag;

					//Insert a Block
					InsertABlock(evicted_victim_cache_set_ptr, evicted_main_block_ptr, victim_block_ptr_front, victim_block_ptr_back);

					//Try fitting block in victim cache
					//BlockLink* evicted_victim_ptr = Fetch_Block_In_A_Set(evicted_victim_cache_set_ptr, evicted_main_block_ptr);

					//If something is evicted delete it

//					if(evicted_victim_ptr!=NULL){
//						if(evicted_victim_ptr->link.dirty==1){
//							p_stats->write_backs++;
//						}
//						p_stats->evictions++;
//						free(evicted_victim_ptr);
//					}
				}

			}
		}

		//No victim cache
		else{
			BlockLink* new_block_ptr = Create_BlockLink(main_tag);
			BlockLink* evicted_main_block_ptr = Fetch_Block_In_A_Set(main_cache_set_ptr,new_block_ptr);
			
			//If write
			if(rw == WRITE){
				//make the block dirty
				new_block_ptr->link.dirty = 1;
			}
			//If something is evicted delete it
			if(evicted_main_block_ptr!=NULL){
				if(evicted_main_block_ptr->link.dirty!=0){
					p_stats->write_backs++;
				}
				p_stats->evictions++;
				free(evicted_main_block_ptr);
			}
		}

		//We got a miss so let us update prefetcher
		if(Cache_Ptr->k!=0){
			Update_Prefetcher(address, Cache_Ptr->b, p_stats);
		}

	}

	//Block Found in Main Cache
	else{
		//Block is refferred make it top priority in main cache
		Update_Block(main_cache_set_ptr,main_cache_block_ptr);

		//If a Write make it dirty
		if(rw==WRITE){
			main_cache_block_ptr->link.dirty = 1;
		}

		if(main_cache_block_ptr->link.prefetched==1){
			//increment useful prefetch
			p_stats->useful_prefetches++;
			main_cache_block_ptr->link.prefetched =2;
		}
	}

}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics
 * such as miss rate or average access time.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */


void complete_cache(cache_stats_t *p_stats) {

	//Get combined misses
	//p_stats->read_misses_combined = p_stats->read_misses - p_stats->vc_read_hits;
	//p_stats->write_misses_combined = p_stats->write_misses- p_stats->vc_write_hits;

	p_stats->miss_rate = (double)(((double)(p_stats->misses))/((double)(p_stats->accesses)));

	p_stats->miss_penalty = 200;
	p_stats->hit_time = 2 + (0.2*Cache_Ptr->s);

	p_stats->avg_access_time = (p_stats->hit_time) + ((p_stats->miss_penalty)*(((double)(p_stats->vc_misses))/((double)(p_stats->accesses))));

	p_stats->bytes_transferred = (p_stats->vc_misses*Cache_Ptr->block_size) + (p_stats->write_backs*Cache_Ptr->block_size) +(p_stats->prefetched_blocks*Cache_Ptr->block_size);

	destroy_cache(Cache_Ptr);
	if(Cache_Ptr->v!=0){

		destroy_cache(Victim_Cache_Ptr);
	}
}

/**
  * Block Node Realised as double Queue
*/
// Create a block of data of given size
char* Create_Block(uint64_t block_size){
  char * block = new char[block_size];
  return block;
}

//Create a block
BlockLink* Create_BlockLink(uint64_t tag_num){
  BlockLink* temp_link = (BlockLink*)malloc(sizeof(BlockLink));
  temp_link->link.tag = tag_num;
  //temp_link->link.block = Create_Block(block_size);
  temp_link->link.valid = 1;
  temp_link->link.dirty = 0;
  temp_link->link.prefetched = 0;
  temp_link->front = temp_link->back = NULL;

  return temp_link;
}

//Fetch a Block and Put it on top of queue
BlockLink* Fetch_Block_In_A_Set(Set* set, BlockLink* block){

	//When blocks per set is just 1, in Case of Direct mapped cache
	if(set->blocks_per_set==1){
		//If Block is occupied remove it
		if(set->filled_blocks==1){
			// Temp block
			BlockLink* temp_block;
			//Use this for return
			temp_block = set->begin;
			//New begin and end for queue
			set->begin = set->end = block;
			//make front and back NULL
			block->back= block->front = NULL;
			//return previous block
			return temp_block;
		}

		//Else if the set is free
		else{
			//Make begin and end point to block
			set->begin = set->end = block;
			//make blocks front and back null
			block->back = block->front = NULL;
			//Increment filled blocks
			set->filled_blocks++;
			//return null
			return NULL;
		}
	}

	//Else is a case of associative caches
	else{
		//Check if Set is empty or full
		if(set->end!=NULL){
			//If not empty
			//Set's begin's front points to new block
			set->begin->front = block;
			//block's front points to NULL
			block->front = NULL;
			//Block's back points to set's begin
			block->back = set->begin;
			//Set's Begin points to new added block(MRU)
			set->begin = block;

			if(set->filled_blocks == set->blocks_per_set){
				BlockLink* temp;
				temp = set->end;
				set->end = set->end->front;
				set->end->back = NULL;
				temp->back = temp->front = NULL;
				return temp;
			}
			else{
				set->filled_blocks++;
				return NULL;
			}

		}
		else{
			set->begin = set->end = block;
			block->back = block->front = NULL;
			set->filled_blocks++;
			return NULL;
		}
	}

}

//Reference a Block in a Set

//Create a set of given blocks
Set* Create_Set(uint64_t s, uint64_t b){
  Set* temp_set;
  temp_set = (Set*)malloc(sizeof(Set));
  temp_set->begin= temp_set->end=NULL;
  temp_set->blocks_per_set = pow(2,s);
  temp_set->block_size = pow(2,b);
  temp_set->b = b;
  temp_set->s = s;
  temp_set->filled_blocks = 0;
  return temp_set;
}


//Check if Block exists for a tag value
BlockLink* Check_Block(Set* check_set, uint64_t tag){
	if(!IfEmptySet(check_set)){
		BlockLink* temp_link = check_set->begin;
		while(temp_link != NULL){
			if(temp_link->link.tag == tag){
				return temp_link;
			}
			else{
				temp_link=temp_link->back;
			}
		}
	}

	return NULL;
}

//A Block is referenced in main cache, update as per LRU
void Update_Block(Set* set, BlockLink* block){

	BlockLink *block_front, *block_back;
	block_front = block->front;
	block_back = block->back;

	//Case: 1 when block is at the top
	if(set->begin == block){
		return;
	}

	//Case 2: When block is at the back
	else if(set->end==block){

		//Subcase: when only two elements
		if(set->filled_blocks==2){

			BlockLink *temp_block = set->begin;
			//Shift begin to end
			set->begin->back = NULL;
			set->begin->front = set->end;
			//Shift end to begin
			set->end->front = NULL;
			set->end->back = set->begin;

			//Swap begin and end
			set->begin = set->end;
			set->end = temp_block;
		}

		else{
			//Make queue's begin now point to end
			set->begin->front = set->end;

			//Detach end from bottom of queue
			set->end->front->back = NULL;
			set->end->front = NULL;

			//Make end's back point to begin
			set->end->back = set->begin;

			//Set new begin to end
			set->begin = set->end;

			//Set new end to end's prev front
			set->end = block_front;
		}
	}

	//Case 3: in middle
	else{
		//Detach block
		block->front->back = block->back;
		block->back->front = block_front;

		//Shift block to front
		block->front = NULL;
		block->back = set->begin;

		//Make begin's front is block
		set->begin->front = block;

		//Set new begin as block
		set->begin = block;
	}
}

//check if Set is empty or not
int IfEmptySet(Set *check_set){
	return(check_set->end==NULL);
}

//Check Tag value
uint64_t GetTag(uint64_t addr, uint64_t c, uint64_t s){

	uint64_t tag = (addr >> (c-s));
	return tag;
}

//Get Index value
uint64_t GetIndex(uint64_t addr, uint64_t c, uint64_t b, uint64_t s){
	uint64_t index;

	index = (addr << (64-(c-s)));
	index = (index >> (64-(c-s)));
	index = (index >> b);
	return index;
}

//Get Address back from index and tag
uint64_t GetAddress(uint64_t tag, uint64_t index, Cache* Cache_Temp_Ptr){
	uint64_t address;
	tag = (tag << (Cache_Temp_Ptr->c - Cache_Temp_Ptr->s));
	index = (index << Cache_Temp_Ptr->b);
	address = (tag|index);

	return address;
}

BlockLink* CheckAddressInACache(uint64_t address, Cache* Cache_Temp_Ptr){

	//Get Tag & Index value from address
	uint64_t tag = GetTag(address, Cache_Temp_Ptr->c, Cache_Temp_Ptr->s);
	uint64_t index = GetIndex(address, Cache_Temp_Ptr->c, Cache_Temp_Ptr->b, Cache_Temp_Ptr->s);

	//Get Desired Set from Cache
	Set* cache_set_ptr = Cache_Temp_Ptr->set_array[index];

	//Check if there is a block in main cache
	BlockLink* cache_block_ptr = Check_Block(cache_set_ptr, tag);

	return cache_block_ptr;
}
void Detach_Block_From_Victim(Set* victim_hit_set, BlockLink* victim_hit_block){

	//Case 1: block is at the end
	if(victim_hit_block->back==NULL){

		//If Only Block Empty Victim Cache
		if(victim_hit_set->filled_blocks==1){
			victim_hit_set->begin = NULL;
			victim_hit_set->end = NULL;
			victim_hit_set->filled_blocks = 0;
			victim_hit_block->back = victim_hit_block->front =NULL;
		}

		//Else Remove this block
		else{
			victim_hit_block->front->back = NULL;
			victim_hit_set->end = victim_hit_block->front;
			victim_hit_block->front = NULL;
			victim_hit_set->filled_blocks--;
		}
	}

	//Case 2: block is in the begining
	else if(victim_hit_block->front==NULL){

		//If only Block then Empty Cache
		if(victim_hit_set->filled_blocks==1){
			victim_hit_set->begin = NULL;
			victim_hit_set->end = NULL;
			victim_hit_set->filled_blocks = 0;
			victim_hit_block->back =  victim_hit_block->front = NULL;
		}

		//Else Remove from front
		else{
			victim_hit_set->begin = victim_hit_block->back;
			victim_hit_block->back->front = NULL;
			victim_hit_block->back = NULL;
			victim_hit_set->filled_blocks--;
		}
	}

	//Case 3: block is in the middle
	else{

		victim_hit_set->filled_blocks--;
		victim_hit_block->front->back = victim_hit_block->back;
		victim_hit_block->back->front = victim_hit_block->front;
		victim_hit_block->back = victim_hit_block->front = NULL;
	}
}

void InsertABlock(Set* set, BlockLink* block, BlockLink* front, BlockLink* back){
		block->front = front;
		block->back = back;

		//Case 1: when block is at the begin of queue
		if(front == NULL){

			//Change queue begin position
			set->begin = block;

			//Check if this was the only block to be inserted, if so then change end of queue too
			if(back == NULL){
				set->end = block;
			}

			//else just make the back point to inserted block
			else{
				block->back->front = block;
			}

			//incrememnt filled block
			set->filled_blocks++;
		}

		//Case2: when block is at the end of queue
		else if(back == NULL){

			//Change the end of the queue
			set->end = block;

			//make the front's back to point to block
			front->back = block;

			//increment filled block
			set->filled_blocks++;
		}

		//Case3: when block is in the middle
		else{

			//Change front's back
			front->back = block;

			//Change back's front
			back->front = block;

			//increment filled blocks
			set->filled_blocks++;
		}

}


void Update_Prefetcher(uint64_t address, uint64_t block_width, cache_stats_t* p_stats){
	address = ((address >> block_width) << block_width);

	//If First Access
	if(Prefetcher_Ptr->first_access == 1){
		//Assign new address
		Prefetcher_Ptr->current_miss_addr = address;
		//Now no more first access
		Prefetcher_Ptr->first_access = 0;
		//Set current stride as address
		Prefetcher_Ptr->current_stride = address;
		//Make negative stride false
		Prefetcher_Ptr->current_negative_stride = false;
		Prefetcher_Ptr->last_negative_stride = false;
	}

	//Else
	else{
		//Set last stride
		Prefetcher_Ptr->last_stride = Prefetcher_Ptr->current_stride;
		//set last miss address
		Prefetcher_Ptr->last_miss_addr = Prefetcher_Ptr->current_miss_addr;
		//Set current miss address
		Prefetcher_Ptr->current_miss_addr = address;
		//Set current stride
		if(Prefetcher_Ptr->current_miss_addr >= Prefetcher_Ptr->last_miss_addr){
			Prefetcher_Ptr->current_stride = Prefetcher_Ptr->current_miss_addr - Prefetcher_Ptr->last_miss_addr;
			//Make negative stride false
			Prefetcher_Ptr->last_negative_stride = Prefetcher_Ptr->current_negative_stride;
			Prefetcher_Ptr->current_negative_stride = false;
		}
		else{
			Prefetcher_Ptr->current_stride = Prefetcher_Ptr->last_miss_addr - Prefetcher_Ptr->current_miss_addr;
			//Make negative stride true
			Prefetcher_Ptr->last_negative_stride = Prefetcher_Ptr->current_negative_stride;
			Prefetcher_Ptr->current_negative_stride = true;
		}

		//If current stride is equal with previous stride, go prefetch
		if((Prefetcher_Ptr->current_stride == Prefetcher_Ptr->last_stride)&&(Prefetcher_Ptr->current_negative_stride == Prefetcher_Ptr->last_negative_stride)){
			//Prefetch it
			p_stats->prefetched_blocks += Prefetcher_Ptr->depth;
			for(uint64_t i = 0; i < Prefetcher_Ptr->depth; i++){
				uint64_t main_index;
				uint64_t main_tag;
				uint64_t prefetch_address;
				//Get prefetch address
				if(Prefetcher_Ptr->current_negative_stride==false){
					prefetch_address = address + ((i+1)*Prefetcher_Ptr->current_stride);
				}
				else{
					prefetch_address = address - ((i+1)*Prefetcher_Ptr->current_stride);
				}
				//Get Tag
				main_tag = GetTag(prefetch_address, Cache_Ptr->c, Cache_Ptr->s);
				//If Fully Associative Index is 0, just one set
				main_index= GetIndex(prefetch_address, Cache_Ptr->c, Cache_Ptr->b, Cache_Ptr->s);

				Set* main_cache_set_ptr = Cache_Ptr->set_array[main_index];
				BlockLink* main_cache_block_ptr = Check_Block(main_cache_set_ptr, main_tag);
				//If Block not present
				if(main_cache_block_ptr==NULL){

					//Victim Cache Present
					if(Cache_Ptr->v!=0){

						//Get Tag & Index value from address
						uint64_t victim_tag = GetTag(prefetch_address, Victim_Cache_Ptr->c, Victim_Cache_Ptr->s);
						uint64_t victim_index = GetIndex(prefetch_address, Victim_Cache_Ptr->c, Victim_Cache_Ptr->b, Victim_Cache_Ptr->s);

						//Get desired set
						Set* victim_cache_set_ptr = Victim_Cache_Ptr->set_array[victim_index];

						//Check if there is a block in victim cache
						BlockLink* victim_cache_block_ptr = Check_Block(victim_cache_set_ptr,victim_tag);

						//if block does not exist in victim go prefetch
						if(victim_cache_block_ptr==NULL){

							//Create a new prefectch block
							BlockLink* prefetch_block_ptr = Create_BlockLink(main_tag);

							//change its prefetch state
							prefetch_block_ptr->link.prefetched = 1;

							//Prefetch in main cache
							BlockLink* evicted_by_prefetch_block_ptr = Prefetch_Block(main_cache_set_ptr, prefetch_block_ptr);

							if(evicted_by_prefetch_block_ptr != NULL){
								uint64_t evicted_by_prefetch_address = GetAddress(evicted_by_prefetch_block_ptr->link.tag, main_index,Cache_Ptr);

								uint64_t evicted_by_prefetch_victim_tag = GetTag(evicted_by_prefetch_address, Victim_Cache_Ptr->c, Victim_Cache_Ptr->s);
								uint64_t evicted_by_prefetch_victim_index = GetIndex(evicted_by_prefetch_address,Victim_Cache_Ptr->c, Victim_Cache_Ptr->b, Victim_Cache_Ptr->s);
								evicted_by_prefetch_block_ptr->link.tag = evicted_by_prefetch_victim_tag;

								Set* evicted_by_prefetch_victim_set_ptr = Victim_Cache_Ptr->set_array[evicted_by_prefetch_victim_index];

								BlockLink* evicted_victim_block_ptr = Fetch_Block_In_A_Set(evicted_by_prefetch_victim_set_ptr, evicted_by_prefetch_block_ptr);

								//Discard the block
								if(evicted_victim_block_ptr != NULL){
									if(evicted_victim_block_ptr->link.dirty==1){
										p_stats->write_backs++;
									}
									p_stats->evictions++;
									free(evicted_victim_block_ptr);
								}
							}
						}

						//If the block exists
						else{

							//Block found in victim,So put it in Main Cache
							//Step1: Remove from victim set
							BlockLink* victim_block_ptr_back = victim_cache_block_ptr->back;
							BlockLink* victim_block_ptr_front = victim_cache_block_ptr->front;
							Detach_Block_From_Victim(victim_cache_set_ptr,victim_cache_block_ptr);

							//Step2: Change tag to main cache tag
							victim_cache_block_ptr->link.tag = main_tag;
							victim_cache_block_ptr->link.prefetched = 1;


							//Step3: PreFetch in Main Cache
							BlockLink* evicted_main_block_ptr = Prefetch_Block(main_cache_set_ptr, victim_cache_block_ptr);
							//Step4: If anything else evicted then put in victim
							if(evicted_main_block_ptr!=NULL){

								//Get evicted address from Main Cache
								uint64_t evicted_address = GetAddress(evicted_main_block_ptr->link.tag, main_index, Cache_Ptr);

								//Get Tag & Index value from address
								uint64_t evicted_victim_tag = GetTag(evicted_address, Victim_Cache_Ptr->c, Victim_Cache_Ptr->s);
								uint64_t evicted_victim_index = GetIndex(evicted_address, Victim_Cache_Ptr->c, Victim_Cache_Ptr->b, Victim_Cache_Ptr->s);
								//Get desired set
								Set* evicted_victim_cache_set_ptr = Victim_Cache_Ptr->set_array[evicted_victim_index];

								//Change tag as per victim cache
								evicted_main_block_ptr->link.tag = evicted_victim_tag;

								//Insert a Block
								InsertABlock(evicted_victim_cache_set_ptr, evicted_main_block_ptr, victim_block_ptr_front, victim_block_ptr_back);
//
//								//Try fitting block in victim cache
//								BlockLink* evicted_victim_ptr = Fetch_Block_In_A_Set(evicted_victim_cache_set_ptr, evicted_main_block_ptr);
//
//								//If something is evicted delete it
//								if(evicted_victim_ptr!=NULL){
//									if(evicted_victim_ptr->link.dirty==1){
//										p_stats->write_backs++;
//									}
//									p_stats->evictions++;
//									free(evicted_victim_ptr);
//								}
							}
						}
					}

					//If no victim cache, Go Prefetch in main
					else{
						//Create a Prefetch block
						BlockLink* prefetch_block_ptr = Create_BlockLink(main_tag);

						//Change block to prefetched
						prefetch_block_ptr->link.prefetched =1;

						//Prefetch in main Cache
						BlockLink* evicted_by_prefetch_ptr = Prefetch_Block(main_cache_set_ptr, prefetch_block_ptr);

						//If something is evicted delete it
						if(evicted_by_prefetch_ptr!=NULL){
							if(evicted_by_prefetch_ptr->link.dirty==1){
								p_stats->write_backs++;
							}
							p_stats->evictions++;
							free(evicted_by_prefetch_ptr);
						}
					}
				}

				//Else include that block in prefetched blocks
				else{
					if(main_cache_block_ptr->link.prefetched == 0){
						//Prefetched but not a useful prefetch as already present
					//main_cache_block_ptr->link.prefetched=2;
					}
				}
			}
		}
	}
}


BlockLink* Prefetch_Block(Set* set, BlockLink* block){

	if(set->blocks_per_set==1){
		if(set->filled_blocks == 1){
			BlockLink* temp_block;
			temp_block = set->begin;
			set->begin= set->end = block;
			block->front = block->back = NULL;
			return temp_block;
		}
		else{
			set->begin= set->end = block;
			block->front = block->back = NULL;
			set->filled_blocks++;
			return NULL;
		}
	}
	else{
		//If not empty
		if(set->end!=NULL){
			//If found full
			if(set->filled_blocks == set->blocks_per_set){
				BlockLink* evicted_block;
				evicted_block = set->end;
				block->front = set->end->front;
				set->end->front->back = block;
				block->back = NULL;
				set->end = block;
				evicted_block->back = NULL;
				evicted_block->front = NULL;
				return evicted_block;
			}
			//Else
			else{
				set->end->back = block;
				block->front = set->end;
				block->back = NULL;
				set->end = block;
				set->filled_blocks++;
				return NULL;
			}
		}
		//If empty
		else{
			set->begin = set->end = block;
			block->front = block->back =NULL;
			set->filled_blocks++;
			return NULL;
		}
	}

}

void destroy_cache(Cache* cache_destroy_ptr){
	for(uint64_t i =0; i<cache_destroy_ptr->num_sets;i++){
		BlockLink* temp = cache_destroy_ptr->set_array[i]->begin;
		BlockLink* temp2;
		while(temp){
			temp2 = temp;
			temp = temp->back;
			free(temp2);
		}
		free(cache_destroy_ptr->set_array[i]);
	}
}

