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
	void* objects;

	// free objects on the slab
	uint32_t free_objects;

	// this is the index of the last allocated object
	uint32_t last_allocated_object;

	// only 1 bit is being used to represent if an object is allocated or not
	// contrary to bonvick's slab alocation where a stack of offsets were used
	// 1 means free object, 0 means occupied in the bitmap
	uint8_t free_bitmap[];
};

typedef struct cache cache;
struct cache;

slab_desc* slab_create(cache* cachep);

void* allocate_object(slab_desc* slab_desc_p, cache* cachep);

// retuns false and fails if the object pointer you tried to free is not valid
int free_object(slab_desc* slab_desc_p, void* object, cache* cachep);

// returns false and fails if the slab objects were being referenced by some user
// i.e. free_object < number of objects on the slab
int slab_destroy(slab_desc* slab_desc_p, cache* cachep);

#endif