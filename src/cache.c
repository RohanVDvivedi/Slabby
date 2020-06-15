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

static void transfer_a_to_b_head(const void* slab_desc_p, linkedlist* a, linkedlist* b)
{
	remove_from_list(a, slab_desc_p);
	insert_head(b, slab_desc_p);
}

static void transfer_a_to_b_tail(const void* slab_desc_p, linkedlist* a, linkedlist* b)
{
	remove_from_list(a, slab_desc_p);
	insert_tail(b, slab_desc_p);
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
	initialize_linkedlist(&(cachep->free_slab_descs), offsetof(slab_desc, slab_list_node));

	pthread_mutex_init(&(cachep->partial_list_lock), NULL);
	initialize_linkedlist(&(cachep->partial_slab_descs), offsetof(slab_desc, slab_list_node));

	pthread_mutex_init(&(cachep->full_list_lock), NULL);
	initialize_linkedlist(&(cachep->full_slab_descs), offsetof(slab_desc, slab_list_node));

	cachep->init = init;
	cachep->recycle = recycle;
	cachep->deinit = deinit;
}

void* cache_alloc(cache* cachep)
{
	if(is_linkedlist_empty(&(cachep->partial_slab_descs)))
	{
		// grow the cache if the free slabs list is also empty
		if(is_linkedlist_empty(&(cachep->free_slab_descs)))
		{
			// increment the free slab list by 2
			cache_grow(cachep);
			cache_grow(cachep);
		}

		// transfer only one slab from free to partial
		transfer_a_to_b_head(get_head(&(cachep->free_slab_descs)), &(cachep->free_slab_descs), &(cachep->partial_slab_descs));
	}

	// get any one slab from the first one third of the slab list, this lessens the lock contention over the same slab by different threads
	// at the same time we aim to not finish up all the slabs in the partial list at the same time
	slab_desc* slab_desc_p = (slab_desc*) get_nth_from_head(&(cachep->partial_slab_descs), ((unsigned int)pthread_self()) % ((cachep->partial_slab_descs.node_count/3) + 1));

	// if there is only one object, on the slab allocate it and mave the slab to full list
	if(slab_desc_p->free_objects == 1)
		transfer_a_to_b_tail(slab_desc_p, &(cachep->partial_slab_descs), &(cachep->full_slab_descs));

	return allocate_object(slab_desc_p, cachep);
}

int cache_free(cache* cachep, void* obj)
{
	// get the page that contains the object
	void* page_addr = (void*)(((uintptr_t)obj) & 0xfff);

	// find some way to find the slab descriptor on which the cureent object is residing
	slab_desc* slab_desc_p = (slab_desc*) find_equals_in_list(&(cachep->full_slab_descs), page_addr, (int (*)(const void *, const void *))is_inside_slab);
	if(slab_desc_p == NULL)
		slab_desc_p = (slab_desc*) find_equals_in_list(&(cachep->partial_slab_descs), page_addr, (int (*)(const void *, const void *))is_inside_slab);

	// if it is in full slabs description, move it to the end of the partial list
	if(exists_in_list(&(cachep->full_slab_descs), slab_desc_p))
	{
		transfer_a_to_b_tail(slab_desc_p, &(cachep->full_slab_descs), &(cachep->partial_slab_descs));
	}
	else if(exists_in_list(&(cachep->partial_slab_descs), slab_desc_p))
	{
		if(slab_desc_p->free_objects == cachep->objects_per_slab - 1)
			transfer_a_to_b_tail(slab_desc_p, &(cachep->partial_slab_descs), &(cachep->free_slab_descs));
	}

	return free_object(slab_desc_p, obj, cachep);
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