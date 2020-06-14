#include<slab.h>

#include<cache.h>
#include<bitmap.h>

#include<strings.h>  /* ffs */
#include<sys/mman.h> /*mmap munmap*/

slab_desc* slab_create(cache* cachep)
{
	// mmap memory equivalent to the size of slab
	void* slab = mmap(NULL, cachep->slab_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED | MAP_POPULATE, 0, 0);

	// this many objects are going to be accomodated in the slab
	uint32_t num_of_objects = number_of_objects_per_slab(cachep);

	// create slab descriptor at the end of the slab
	slab_desc* slab_desc_p = (slab + cachep->slab_size) - (bitmap_size_in_bytes(num_of_objects) + sizeof(slab_desc));

	// initialize attributes, note : slab_desc object is kept at the end of the slab
	// slab objects always start at the beginning
	slab_desc_p->objects = slab;
	pthread_mutex_init(&(slab_desc_p->slab_lock), NULL);
	slab_desc_p->prev = NULL;
	slab_desc_p->next = NULL;
	slab_desc_p->free_objects = num_of_objects;
	slab_desc_p->last_allocated_object = 0;

	// initialize the free bitmap, set all bits to 1 to indicate that they are free
	set_all_bits(slab_desc_p->free_bitmap, num_of_objects);

	// init all the objects
	for(uint32_t i = 0; i < num_of_objects; i++)
	{
		cachep->init(slab_desc_p->objects + (i * cachep->object_size));
	}

	return slab_desc_p;
}

void* allocate_object(slab_desc* slab_desc_p, cache* cachep)
{
	// this many objects are going to be accomodated in the slab
	uint32_t num_of_objects = number_of_objects_per_slab(cachep);print_bitmap(slab_desc_p->free_bitmap, num_of_objects);

	void* object = NULL;

	pthread_mutex_lock(&(slab_desc_p->slab_lock));

		// get a free object if there exists any
		if(slab_desc_p->free_objects)
		{
			uint32_t object_index = find_first_set(slab_desc_p->free_bitmap, slab_desc_p->last_allocated_object, num_of_objects);

			// this effectively lets us know that there is a free object close by the last allocated index
			// there is generally a free object close by
			slab_desc_p->last_allocated_object = object_index;

			// mark the object as allocated / 0 in the allocation bit map size, and decrement the count of free objects
			reset_bit(slab_desc_p->free_bitmap, object_index);
			slab_desc_p->free_objects--;

			object = slab_desc_p->objects + (cachep->object_size * object_index);
		}

	pthread_mutex_unlock(&(slab_desc_p->slab_lock));

	return object;
}

int free_object(slab_desc* slab_desc_p, void* object, cache* cachep)
{
	if(!(slab_desc_p->objects <= object && object < ((void*)slab_desc_p)))
		return 0;

	if(((((uintptr_t)object) - ((uintptr_t)slab_desc_p->objects)) % cachep->object_size))
		return 0;

	pthread_mutex_lock(&(slab_desc_p->slab_lock));

		// call recycle 
		cachep->recycle(object);

		uint32_t object_index = (((uintptr_t)object) - ((uintptr_t)slab_desc_p->objects)) / cachep->object_size;

		// this effectively lets us know that there is a free object close by the last allocated index
		if(slab_desc_p->free_objects == 0)
			slab_desc_p->last_allocated_object = object_index;

		// set the fre bitmap to mark the object as free
		set_bit(slab_desc_p->free_bitmap, object_index);
		slab_desc_p->free_objects++;

	pthread_mutex_unlock(&(slab_desc_p->slab_lock));

	return 1;
}

int slab_destroy(slab_desc* slab_desc_p, cache* cachep)
{
	// this many objects are going to be accomodated in the slab
	uint32_t num_of_objects = number_of_objects_per_slab(cachep);

	if(slab_desc_p->free_objects < num_of_objects)
		return 0;

	// iterate over all the objects and deinit all of them
	for(uint32_t i = 0; i < num_of_objects; i++)
		cachep->deinit(slab_desc_p->objects + (i * cachep->object_size));

	pthread_mutex_destroy(&(slab_desc_p->slab_lock));

	// munmap memory
	munmap(slab_desc_p->objects, cachep->slab_size);

	return 1;
}