#ifndef SLAB_H
#define SLAB_H

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>

typedef struct slab_desc slab_desc;
struct slab_desc
{
	pthread_mutex_t slab_lock;

	slab_desc* next;
	slab_desc* prev;

	// pointer to the first object
	// ith object => (i < number_of_objects_per_slab) ? (objects + (object_size * i)) : NULL;
	void* objects;

	// number_of_objects_per_slab = ( (8*(slab_size-sizeof(slab_desc))) / ((8*object_size)+1))
	// extra 1 bit corresponds to check if the object is allocated to user or not using the allocation bitmap

	// reference count of the slab, a slab is considered free and is put in free list if it's reference count is 0
	// i.e. it is not in use by any user thread
	uint32_t reference_count;

	// this is the index of the last object in slab which was allocated
	uint32_t last_allocation_index;

	// only 1 bit is being used to represent if an object is allocated or not
	// contrary to bonvick's slab alocation where a stack of offsets were used
	// 1 means free object, 0 means occupied
	uint8_t allocation_bit_map[];
};

typedef struct cache cache;
struct cache;

slab_desc* slab_create(cache* cachep);

void* allocate_object(slab_desc* slab_desc_p, cache* cachep);

void free_object(slab_desc* slab_desc_p, void* object, cache* cachep);

void slab_destroy(slab_desc* slab_desc_p, cache* cachep);

#endif