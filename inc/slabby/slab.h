#ifndef SLAB_H
#define SLAB_H

#include<pthread.h>

#include<cutlery/linkedlist.h>

// slab_desc is short for slab_description
// this structure is always present at the start of the slab
// a slab is a group of some contiguous 4KB pages
// the objects in the slab are pushed to occupy the space aligning with the end of slab

typedef struct slab_desc slab_desc;
struct slab_desc
{
	// exclusive access to the list node is protected and locked by the cache_lock of the cache
	// node used to link slab_desc
	llnode slab_list_node;

	// **** slab attributes

	// pointer to the first object
	void* objects;

	// free objects on the slab
	size_t free_objects;

	// this is the index of the last allocated object
	size_t last_allocated_object;

	// only 1 bit is being used to represent if an object is allocated or not
	// contrary to bonvick's slab alocation where a stack of offsets were used
	// 1 means free object, 0 means occupied in the bitmap
	char free_bitmap[];
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