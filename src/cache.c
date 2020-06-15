#include<cache.h>

#include<stddef.h>

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

static uint32_t number_of_objects_per_slab(cache* cachep)
{
	// number of unused bits in slab after alloting slab_description structure
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

	cachep->objects_per_slab = number_of_objects_per_slab(cachep);

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

void* cache_alloc(cache* cachep)
{
	void* object = NULL;

	if(is_linkedlist_empty(&(cachep->partial_slab_descs)))
	{
		// grow the cache
	}
	slab_desc* slab_desc_p = (slab_desc*) get_nth_from_head(&(cachep->partial_slab_descs), ((unsigned int)pthread_self()) % (cachep->partial_slab_descs.node_count/2));

	object = allocate_object(slab_desc_p, cachep);
	if(slab_desc_p->free_objects == 1)
	{
		remove_from_list(&(cachep->partial_slab_descs), slab_desc_p);
		insert_head(&(cachep->full_list_lock), slab_desc_p);
	}

	return object;
}

void cache_free(cache* cachep, void* obj)
{

}

void cache_grow(cache* cachep)
{
	slab_desc* slab_desc_p = slab_create(cachep);
	insert_head(&(cachep->free_slab_descs), slab_desc_p);
}

int cache_reap(cache* cachep)
{
	slab_desc* slab_desc_p = NULL;
	if(!is_linkedlist_empty(&(cachep->free_slab_descs)))
	{
		slab_desc_p = (slab_desc*) get_head(&(cachep->free_slab_descs));
		remove_head(&(cachep->free_slab_descs));
	}
	if(slab_desc_p)
	{
		slab_destroy(slab_desc_p, cachep);
		return 1;
	}
	return 0;
}

int cache_destroy(cache* cachep)
{
	if(cachep->partial_slab_descs.node_count || cachep->full_slab_descs.node_count)
		return 0;

	while(!is_linkedlist_empty(&(cachep->free_slab_descs)))
	{
		slab_desc* slab_desc_p = (slab_desc*) get_head(&(cachep->free_slab_descs));
		remove_head(&(cachep->free_slab_descs));
		slab_destroy(slab_desc_p, cachep);
	}

	pthread_mutex_destroy(&(cachep->free_list_lock));
	pthread_mutex_destroy(&(cachep->partial_list_lock));
	pthread_mutex_destroy(&(cachep->full_list_lock));

	return 1;
}