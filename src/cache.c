#include<cache.h>

#include<slab.h>

#include<stdint.h>
#include<stddef.h>

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

uint32_t number_of_objects_per_slab(cache* cachep)
{
	// number of unused bits in slab after alloting slab_description structure
	// divided by the number of bits that will be occupied by the object
	// each object will occupy 1 bot to store if it is allocated to user or is free
	return ( 8 * (cachep->slab_size - sizeof(slab_desc)) ) / (8 * cachep->object_size + 1);
}

static void transfer_a_to_b_head(const void* slab_desc_p, linkedlist* a, linkedlist* b)
{
	remove_from_linkedlist(a, slab_desc_p);
	insert_head(b, slab_desc_p);
}

static void transfer_a_to_b_tail(const void* slab_desc_p, linkedlist* a, linkedlist* b)
{
	remove_from_linkedlist(a, slab_desc_p);
	insert_tail(b, slab_desc_p);
}

static void cache_grow_unsafe(cache* cachep)
{
	slab_desc* slab_desc_p = slab_create(cachep);
	insert_head(&(cachep->free_slab_descs), slab_desc_p);
	cachep->free_slabs++;
}

static int cache_reap_unsafe(cache* cachep)
{
	slab_desc* slab_desc_p = NULL;
	if(!is_empty_linkedlist(&(cachep->free_slab_descs)))
	{
		slab_desc_p = (slab_desc*) get_head(&(cachep->free_slab_descs));
		remove_head(&(cachep->free_slab_descs));
		cachep->free_slabs--;
	}
	if(slab_desc_p)
	{
		slab_destroy(slab_desc_p, cachep);
		return 1;
	}
	return 0;
}

void cache_create(	cache* cachep,

					size_t slab_size,
					size_t object_size,

					void (*init)(void*, size_t),
					void (*deinit)(void*, size_t)
				)
{
	pthread_mutex_init(&(cachep->cache_lock), NULL);

	// always a multiple of 4096
	cachep->slab_size = ((slab_size/4096)*4096) + ((slab_size%4096)?4096:0);
	cachep->object_size = object_size;

	cachep->free_slabs = 0;
	initialize_linkedlist(&(cachep->free_slab_descs), offsetof(slab_desc, slab_list_node));
	cachep->partial_slabs = 0;
	initialize_linkedlist(&(cachep->partial_slab_descs), offsetof(slab_desc, slab_list_node));
	cachep->full_slabs = 0;
	initialize_linkedlist(&(cachep->full_slab_descs), offsetof(slab_desc, slab_list_node));

	cachep->init = init;
	cachep->deinit = deinit;
}

void* cache_alloc(cache* cachep)
{
	pthread_mutex_lock(&(cachep->cache_lock));

	if(is_empty_linkedlist(&(cachep->partial_slab_descs)))
	{
		// grow the cache if the free slabs list is also empty
		if(is_empty_linkedlist(&(cachep->free_slab_descs)))
		{
			// increment the free slab list by 2
			cache_grow_unsafe(cachep);
			cache_grow_unsafe(cachep);
		}

		// transfer only one slab from free to partial
		transfer_a_to_b_head(get_head(&(cachep->free_slab_descs)), &(cachep->free_slab_descs), &(cachep->partial_slab_descs));
		cachep->free_slabs--;
		cachep->partial_slabs++;
	}

	// get any one slab from the first one third of the slab list, this lessens the lock contention over the same slab by different threads
	// at the same time we aim to not finish up all the slabs in the partial list at the same time
	unsigned int slab_to_pick = (((unsigned int)pthread_self()) % cachep->partial_slabs)/3;
	slab_desc* slab_desc_p = (slab_desc*) get_nth_from_head(&(cachep->partial_slab_descs), slab_to_pick);

	// lock the slab asap after you get the pointer to it
	lock_slab(slab_desc_p);

	// if there is only one object, on the slab allocate it and mave the slab to full list
	if(slab_desc_p->free_objects == 1)
	{
		transfer_a_to_b_tail(slab_desc_p, &(cachep->partial_slab_descs), &(cachep->full_slab_descs));
		cachep->partial_slabs--;
		cachep->full_slabs++;
	}

	pthread_mutex_unlock(&(cachep->cache_lock));

	void* object = allocate_object(slab_desc_p, cachep);

	unlock_slab(slab_desc_p);

	return object;
}

int cache_free(cache* cachep, void* obj)
{
	// get the page that contains the object
	void* page_addr = (void*)(((uintptr_t)obj) & ~(0xfff));

	pthread_mutex_lock(&(cachep->cache_lock));

	int exists_full_slabs = 0;
	int exists_partial_slabs = 0;

	// find some way to find the slab descriptor on which the cureent object is residing
	slab_desc* slab_desc_p = (slab_desc*) find_equals_in_linkedlist(&(cachep->full_slab_descs), page_addr, (int (*)(const void *, const void *))is_inside_slab);
	if(slab_desc_p == NULL)
	{
		slab_desc_p = (slab_desc*) find_equals_in_linkedlist(&(cachep->partial_slab_descs), page_addr, (int (*)(const void *, const void *))is_inside_slab);
		exists_partial_slabs = 1;
	}
	else
		exists_full_slabs = 1;

	// lock the slab asap after you get the pointer to it
	lock_slab(slab_desc_p);

	// if it is in full slabs description, move it to the end of the partial list
	if(exists_full_slabs)
	{
		transfer_a_to_b_tail(slab_desc_p, &(cachep->full_slab_descs), &(cachep->partial_slab_descs));
		cachep->full_slabs--;
		cachep->partial_slabs++;
	}
	else if(exists_partial_slabs)
	{
		if(slab_desc_p->free_objects == number_of_objects_per_slab(cachep) - 1)
		{
			transfer_a_to_b_tail(slab_desc_p, &(cachep->partial_slab_descs), &(cachep->free_slab_descs));
			cachep->partial_slabs--;
			cachep->free_slabs++;
		}
	}

	if(cachep->partial_slabs < cachep->free_slabs)
		cache_reap_unsafe(cachep);

	pthread_mutex_unlock(&(cachep->cache_lock));

	int freed = free_object(slab_desc_p, obj, cachep);

	unlock_slab(slab_desc_p);

	return freed;
}

void cache_grow(cache* cachep)
{
	pthread_mutex_lock(&(cachep->cache_lock));
		cache_grow_unsafe(cachep);
	pthread_mutex_unlock(&(cachep->cache_lock));
}

int cache_reap(cache* cachep)
{
	pthread_mutex_lock(&(cachep->cache_lock));
		int reaped = cache_reap_unsafe(cachep);
	pthread_mutex_unlock(&(cachep->cache_lock));
	return reaped;
}

int cache_destroy(cache* cachep)
{
	if(cachep->partial_slabs || cachep->full_slabs)
		return 0;

	int reaped = 0;

	while(!is_empty_linkedlist(&(cachep->free_slab_descs)))
		reaped += cache_reap_unsafe(cachep);

	pthread_mutex_destroy(&(cachep->cache_lock));

	return 1 + reaped;
}