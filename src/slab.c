#include<slab.h>

#include<cache.h>
#include<bitmap.h>

#include<strings.h>  /* ffs */
#include<sys/mman.h> /*mmap munmap*/

static uint32_t allocation_bit_map_size(uint32_t object_count)
{
	return (object_count/8) + (((object_count % 8)>0) ? 1 : 0);
}

slab_desc* slab_create(cache* cachep)
{
	// mmap memory equivalent to the size of slab
	void* slab = mmap(NULL, cachep->slab_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED | MAP_POPULATE, 0, 0);

	// this many objects are going to be accomodated in the slab
	uint32_t num_of_objects = number_of_objects_per_slab(cachep);

	// size of allocation bit map in bytes
	uint32_t alloc_bit_map_size = allocation_bit_map_size(num_of_objects);

	// create slab descriptor at the end of the slab
	slab_desc* slab_desc_p = (slab + cachep->slab_size) - (alloc_bit_map_size + sizeof(slab_desc));

	// initialize attributes, note : slab_desc object is kept at the end of the slab
	// slab objects always start at the beginning
	slab_desc_p->objects = slab;
	slab_desc_p->prev = NULL;
	slab_desc_p->next = NULL;
	slab_desc_p->reference_count = 0;
	slab_desc_p->last_allocation_bit_map_index = 0;

	// initialize the allocation bitmap, set all bits to 1 to indicate that they are free
	for(uint32_t i = 0; i < alloc_bit_map_size; i++)
	{
		slab_desc_p->allocation_bit_map[i] = 0xff;
	}

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
	uint32_t num_of_objects = number_of_objects_per_slab(cachep);

	// size of allocation bit map in bytes
	uint32_t alloc_bit_map_size = allocation_bit_map_size(num_of_objects);

	// if the object is found we will hols a valid value in object_index, 
	// while will point to the object in the object array in the slab that will be allocated
	int object_found = 0;
	uint32_t object_index;

	// bitmap index will start from the 
	uint32_t bit_map_index = slab_desc_p->last_allocation_bit_map_index;

	// loop over allocation_bit_map[] and find the free object
	for(uint32_t i = 0; i < alloc_bit_map_size; i++)
	{
		if(slab_desc_p->allocation_bit_map[bit_map_index] > 0)
		{
			object_index = bit_map_index * 8 + ffs(slab_desc_p->allocation_bit_map[bit_map_index]) - 1;
			if(object_index < num_of_objects)
			{
				object_found = 1;
				break;
			}
		}

		bit_map_index = (bit_map_index + 1) % alloc_bit_map_size;
	}

	if(object_found)
	{
		// mark the object as allocated / 0 in the allocation bit map size
		reset_bitmap(slab_desc_p->allocation_bit_map, object_index);

		// increment the reference count of the slab
		slab_desc_p->reference_count++;

		// set last_allocation_bit_map_index accordingly
		slab_desc_p->last_allocation_bit_map_index = bit_map_index;

		return slab_desc_p->objects + (object_index * cachep->object_size);
	}

	return NULL;
}

int free_object(slab_desc* slab_desc_p, void* object, cache* cachep)
{
	if(!(slab_desc_p->objects <= object && object < ((void*)slab_desc_p)))
	{
		return 0;
	}

	if((object - slab_desc_p->objects) % cachep->object_size != 0)
	{
		return 0;
	}

	// call recycle 
	cachep->recycle(object);

	uint32_t object_index = (((uintptr_t)object) - ((uintptr_t)slab_desc_p->objects)) / cachep->object_size;

	// set the allocation bit map to mark the object as free
	set_bitmap(slab_desc_p->allocation_bit_map, object_index);

	// decrement the reference count of the slab
	slab_desc_p->reference_count++;

	return 1;
}

int slab_destroy(slab_desc* slab_desc_p, cache* cachep)
{
	if(slab_desc_p->reference_count != 0)
	{
		return 0;
	}

	// iterate over all the objects and deinit all of them
	uint32_t num_of_objects = number_of_objects_per_slab(cachep);
	for(uint32_t i = 0; i < num_of_objects; i++)
	{
		cachep->deinit(slab_desc_p->objects + (i * cachep->object_size));
	}

	// munmap memory
	munmap(slab_desc_p->objects, cachep->slab_size);

	return 1;
}