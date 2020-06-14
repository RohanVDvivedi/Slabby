#include<cache.h>

uint32_t number_of_objects_per_slab(cache* cachep)
{
	// number of bits vacant in object after alloting slab_description structure
	// divided by the number of bits that will be occupied by the object
	// each object will occupy 1 bot to store if it is allocated to user or is free
	return ( 8 * (cachep->slab_size - sizeof(slab_desc)) ) / (8 * cachep->object_size + 1);
}

int cache_create(cache* cachep, size_t slab_size, size_t object_size, void (*init)(void*), void (*recycle)(void*), void (*deinit)(void*));

void* cache_alloc(cache* cachep);
void cache_free(cache* cachep, void* obj);

void cache_grow(cache* cachep, uint32_t slabs_to_add);
uint32_t cache_reap(cache* cachep);

int cache_destroy(cache* cachep);