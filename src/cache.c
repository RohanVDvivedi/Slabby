#include<cache.h>

#include<stddef.h>

uint32_t number_of_objects_per_slab(cache* cachep)
{
	// number of bits vacant in object after alloting slab_description structure
	// divided by the number of bits that will be occupied by the object
	// each object will occupy 1 bot to store if it is allocated to user or is free
	return ( 8 * (cachep->slab_size - sizeof(slab_desc)) ) / (8 * cachep->object_size + 1);
}

void cache_create(	cache* cachep,

					size_t slab_size,
					size_t object_size,

					void (*init)(void*, size_t),
					void (*recycle)(void*, size_t),
					void (*deinit)(void*, size_t)
				)
{
	// always a multiple of 4096
	cachep->slab_size = ((slab_size/4096)*4096) + ((slab_size%4096)?4096:0);

	cachep->object_size = object_size;

	pthread_mutex_init(&(cachep->free_list_lock), NULL);
	initialize_linkedlist(&(cachep->free_slab_descs), offsetof(slab_desc, slab_list_node), NULL);

	pthread_mutex_init(&(cachep->partial_list_lock), NULL);
	initialize_linkedlist(&(cachep->partial_slab_descs), offsetof(slab_desc, slab_list_node), NULL);

	pthread_mutex_init(&(cachep->full_list_lock), NULL);
	initialize_linkedlist(&(cachep->full_slab_descs), offsetof(slab_desc, slab_list_node), NULL);

	cachep->init = init;
	cachep->recycle = recycle;
	cachep->deinit = deinit;
}

void* cache_alloc(cache* cachep);
void cache_free(cache* cachep, void* obj);

void cache_grow(cache* cachep, uint32_t slabs_to_add);
uint32_t cache_reap(cache* cachep);

int cache_destroy(cache* cachep)
{
	if(cachep->partial_slab_descs.node_count || cachep->full_slab_descs.node_count)
		return 0;

	pthread_mutex_lock(&(cachep->free_list_lock));
		while(!is_linkedlist_empty(&(cachep->free_slab_descs)))
		{
			slab_desc* slab_desc_p = (slab_desc*) get_head(&(cachep->free_slab_descs));
			remove_head(&(cachep->free_slab_descs));
			slab_destroy(slab_desc_p, cachep);
		}
	pthread_mutex_unlock(&(cachep->free_list_lock));

	pthread_mutex_destroy(&(cachep->free_list_lock));
	pthread_mutex_destroy(&(cachep->partial_list_lock));
	pthread_mutex_destroy(&(cachep->full_list_lock));

	return 1;
}