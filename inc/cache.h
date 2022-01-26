#ifndef CACHE_H
#define CACHE_H

#include<pthread.h>
#include<stdio.h>

#include<singlylist.h>
#include<linkedlist.h>

typedef struct cache cache;
struct cache
{
	pthread_mutex_t	cache_lock;		// protects all the three linkedlists and their corresponding slab counts from concurrent access

	unsigned int free_slabs;
	singlylist free_slab_descs;			// list of free slabs ready to reap, no objects are currently in user space from here

	unsigned int partial_slabs;
	linkedlist partial_slab_descs;		// list of partial slabs, for further allocation

	unsigned int full_slabs;
	linkedlist full_slab_descs;			// list of full slabs, a slab is moved to-fro free and full slabs as required

	size_t slab_size;			// size of each slab in bytes, must always be multiple of 4096 and 
								// greater than atleast 32 times the size of the object

	size_t object_size;			// size of each object in bytes, this must always be a multiple of 64

	// number_of_objects_per_slab = ( (8*(slab_size-sizeof(slab_desc))) / ((8*object_size)+1))
	// 1 bit for allocation mapping, to check if a object is allocated or not

	void (*init)(void*, size_t);		// called on all objects when a new slab is added to the cache
	void (*deinit)(void*, size_t);		// called on all objects before returning a slab to the os
		// in all the above functions the void* typed frst parameter will be the pointer to the object,
		// while size_t typed second parameter will the the object_size of the cache
};

void cache_create(cache* cachep, size_t slab_size, size_t object_size, void (*init)(void*, size_t), void (*deinit)(void*, size_t));

void* cache_alloc(cache* cachep);
int cache_free(cache* cachep, void* obj);			// returns true if the object was freed

void cache_grow(cache* cachep);
int cache_reap(cache* cachep);						// returns 1, if a slab was reaped

int cache_destroy(cache* cachep);

/*
**	NO CONCURRENT CACHE CREATE AND DESTROY CALLS MUST BE MADE ON THE cache
**	so cache must be created and destroyed by your application's main thread
**	cache_lock is used only for alloc, free, reap and destroy operation
*/

#endif