#ifndef CACHE_H
#define CACHE_H

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>

#include<slab.h>

typedef struct cache cache;
struct cache
{
	pthread_mutex_t	free_list_lock;		// allows mutually exclusive access to the free slabs list
	slab_desc* free;			// list of free slabs ready to reap, no objects are currently in user space from here
	uint32_t free_slabs;		// number of slabs in 

	pthread_mutex_t	partial_list_lock;	// allows mutually exclusive access to the partial slabs list
	slab_desc* partial;			// list of partial slabs, for further allocation
	uint32_t partial_slabs;		// number of partial slabs

	pthread_mutex_t full_list_lock; 	// allows mutually exclusive access to the full slabs list
	slab_desc* full;			// list of full slabs, a slab is moved to-fro free and full slabs as required
	uint32_t full_slabs;		// number of full slabs

	uint32_t min_in_use_slabs;	// if the number of partial + full slabs geos below this number 
								// a new slab is moved from free to partial slabs and the new object is allocated from there
	uint32_t max_free_slabs;	// free slabs are automatically returned if they exceeed max_free_slabs

	size_t slab_size;			// size of each slab in bytes, must always be multiple of 4096 and 
								// greater than atleast 32 times the size of the object

	size_t object_size;			// size of each object in bytes, this must always be a multiple of 64

	volatile int growing;		// set when a new slab is being created and will be added to free list soon

	// number_of_objects_per_slab = ( (8*(slab_size-sizeof(slab_desc))) / ((8*object_size)+1))
	// 1 bit for allocation mapping, to check if a object is allocated or not

	void (*init)(void*);		// called on all objects when a new slab is added to the cache
	void (*recycle)(void*);		// called before giving a used object again to the user
	void (*deinit)(void*);		// called on all objects before returning a slab to the os
};

uint32_t number_of_objects_per_slab(cache* cachep);

int cache_create(cache* cachep, size_t slab_size, size_t object_size, void (*init)(void*), void (*recycle)(void*), void (*deinit)(void*));

void* cache_alloc(cache* cachep);
void cache_free(cache* cachep, void* obj);

void cache_grow(cache* cachep, uint32_t slabs_to_add);
uint32_t cache_reap(cache* cachep);						// returns the number of slabs reaped

int cache_destroy(cache* cachep);

#endif