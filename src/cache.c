#include<slabby/cache.h>

#include<slabby/slab.h>

#include<stdint.h>
#include<stddef.h>

size_t number_of_objects_per_slab(cache* cachep)
{
	// number of unused bits in slab after alloting slab_description structure
	// divided by the number of bits that will be occupied by the object
	// each object will occupy 1 bit to store if it is allocated to user or is free
	return ( CHAR_BIT * (cachep->slab_size - sizeof(slab_desc)) ) / (CHAR_BIT * cachep->object_size + 1);
}

static size_t get_cache_memory_hoarded_unsafe(cache* cachep)
{
	return (cachep->free_slabs + cachep->partial_slabs + cachep->full_slabs) * cachep->slab_size;
}

static int cache_grow_unsafe(cache* cachep)
{
	if(cachep->max_memory_hoarding != 0 && cachep->max_memory_hoarding <= get_cache_memory_hoarded_unsafe(cachep))
		return 0;

	slab_desc* slab_desc_p = slab_create(cachep);
	if(slab_desc_p == NULL) // cache grow fails, if slab_create fails
		return 0;

	insert_head_in_singlylist(&(cachep->free_slab_descs), slab_desc_p);
	cachep->free_slabs++;
	return 1;
}

static int cache_reap_unsafe(cache* cachep)
{
	if(!is_empty_singlylist(&(cachep->free_slab_descs)))
	{
		slab_desc* slab_desc_p = (slab_desc*) get_head_of_singlylist(&(cachep->free_slab_descs));
		remove_head_from_singlylist(&(cachep->free_slab_descs));
		cachep->free_slabs--;
		slab_destroy(slab_desc_p, cachep);
		return 1;
	}
	return 0;
}

void cache_create(	cache* cachep,

					size_t slab_size,
					size_t object_size,

					size_t max_memory_hoarding,

					void (*init)(void*, size_t),
					void (*deinit)(void*, size_t)
				)
{
	pthread_mutex_init(&(cachep->cache_lock), NULL);

	// always a multiple of 4096
	cachep->slab_size = (((slab_size+4095)/4096)*4096);	// equivalent to multiple of 4096 more than or equal to slab_size
	cachep->object_size = object_size;
	cachep->max_memory_hoarding = (((max_memory_hoarding+4095)/4096)*4096);

	cachep->free_slabs = 0;
	initialize_singlylist(&(cachep->free_slab_descs), offsetof(slab_desc, slab_list_node));
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
		if(is_empty_singlylist(&(cachep->free_slab_descs)))
		{
			// increment the free slab list, by allocating a new slab
			if(0 == cache_grow_unsafe(cachep))
			{
				// if we couldn't add a new slab (for any reason) then we can not allocate memory now
				// since both partial_slabs and full_slabs count are 0
				pthread_mutex_unlock(&(cachep->cache_lock));
				return NULL;
			}

			// since we might need more memory in future
			// we add one more slab
			// this may or may not succeed, we don't care
			cache_grow_unsafe(cachep);
		}

		// transfer only one slab from free to partial
		const slab_desc* slab_desc_to_move = get_head_of_singlylist(&(cachep->free_slab_descs));
		remove_head_from_singlylist(&(cachep->free_slab_descs));
		cachep->free_slabs--;
		insert_head_in_linkedlist(&(cachep->partial_slab_descs), slab_desc_to_move);
		cachep->partial_slabs++;
	}

	// get any one slab from the first one third of the slab list
	// we aim to not finish up all the slabs in the partial list at the same time
	// this designed allowed concurrency for several concurrent allocations in separate slabs, but this reasoning is now obsolete
	size_t slab_to_pick = (((size_t)pthread_self()) % cachep->partial_slabs)/3;
	slab_desc* slab_desc_p = (slab_desc*) get_from_head_of_linkedlist(&(cachep->partial_slab_descs), slab_to_pick);

	// attempt to allocate the object
	void* object = allocate_object(slab_desc_p, cachep);
	// if allocation failed, then we do not need to do any maintainance, of transfer of slab_desc_p
	if(object == NULL)
	{
		pthread_mutex_unlock(&(cachep->cache_lock));
		return NULL;
	}

	// if there are no free objects, on the slab, then move it to full_slab_descs
	if(slab_desc_p->free_objects == 0)
	{
		remove_from_linkedlist(&(cachep->partial_slab_descs), slab_desc_p);
		cachep->partial_slabs--;
		insert_tail_in_linkedlist(&(cachep->full_slab_descs), slab_desc_p);
		cachep->full_slabs++;
	}

	pthread_mutex_unlock(&(cachep->cache_lock));

	return object;
}

int cache_free(cache* cachep, void* obj)
{
	pthread_mutex_lock(&(cachep->cache_lock));

	int exists_in_full_slabs = 0;
	int exists_in_partial_slabs = 0;

	// since all the slabs are aligned to their sizes, we get the pointer to the slab of this object by :
	void* slab = obj - (((uintptr_t)obj) % cachep->slab_size);

	// since slab_desc is at the beginning of the slab,
	// so if we know the slab, we know its slab_desc
	slab_desc* slab_desc_p = slab;

	// figure out the linkedlist of this slab
	if(slab_desc_p->free_objects > 0)
		exists_in_partial_slabs = 1;
	else
		exists_in_full_slabs = 1;

	// attempt to free the object
	int freed = free_object(slab_desc_p, obj, cachep);
	// if deallocation failed, then we do not need to do any maintainance, of transfer of slab_desc_p
	if(freed == 0)
	{
		pthread_mutex_unlock(&(cachep->cache_lock));
		return 0;
	}

	// if it is in full slabs description, move it to the end of the partial list
	if(exists_in_full_slabs)
	{
		remove_from_linkedlist(&(cachep->full_slab_descs), slab_desc_p);
		cachep->full_slabs--;
		insert_tail_in_linkedlist(&(cachep->partial_slab_descs), slab_desc_p);
		cachep->partial_slabs++;
	}
	else if(exists_in_partial_slabs)
	{
		if(slab_desc_p->free_objects == number_of_objects_per_slab(cachep))
		{
			remove_from_linkedlist(&(cachep->partial_slab_descs), slab_desc_p);
			cachep->partial_slabs--;
			insert_head_in_singlylist(&(cachep->free_slab_descs), slab_desc_p);
			cachep->free_slabs++;
		}
	}

	pthread_mutex_unlock(&(cachep->cache_lock));


	// a small attempt to free up memory and return to OS, if there are more free slabs then required
	pthread_mutex_lock(&(cachep->cache_lock));

	while(cachep->partial_slabs < cachep->free_slabs)
		cache_reap_unsafe(cachep);

	pthread_mutex_unlock(&(cachep->cache_lock));

	return freed;
}

int cache_grow(cache* cachep)
{
	pthread_mutex_lock(&(cachep->cache_lock));
		int grew = cache_grow_unsafe(cachep);
	pthread_mutex_unlock(&(cachep->cache_lock));
	return grew;
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

	while(!is_empty_singlylist(&(cachep->free_slab_descs)))
		reaped += cache_reap_unsafe(cachep);

	pthread_mutex_destroy(&(cachep->cache_lock));

	return 1 + reaped;
}

size_t get_cache_memory_hoarded(cache* cachep)
{
	pthread_mutex_lock(&(cachep->cache_lock));
		size_t hoarded_memory = get_cache_memory_hoarded_unsafe(cachep);
	pthread_mutex_unlock(&(cachep->cache_lock));
	return hoarded_memory;
}