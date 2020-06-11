#include<slab.h>

#include<cache.h>

static uint32_t allocation_bit_map_size(uint32_t object_count)
{
	return (object_count/8) + (((object_count % 8)>0) ? 1 : 0);
}

static void set_allocation_bit_map(uint8_t* allocation_bit_map, uint32_t object_no)
{
	allocation_bit_map[object_no/8] |= (1<<(object_no%8));
}

static void reset_allocation_bit_map(uint8_t* allocation_bit_map, uint32_t object_no)
{
	allocation_bit_map[object_no/8] &= (~(1<<(object_no%8)));
}

slab_desc* slab_create(cache* cachep)
{
	// mmap memory equivalent to the size of slab
	// initialize attributes, note : slab_desc object is kept at the end of the slab
	// initialize the allocation bitmap, set all bits to 1 to indicate that they are free
	// init all the objects
	return NULL;
}

void* allocate_object(slab_desc* slab_desc_p, cache* cachep)
{
	// loop over allocation_bit_map[]
	// find the free object
	// mark the object as allocated / 0 in the allocation bit map size
	// increment the reference count of the slab
	// set last_allocation_index accordingly
	return NULL;
}

void free_object(slab_desc* slab_desc_p, void* object, cache* cachep)
{
	// call recycle 
	// decrement the reference count of the slab
}

void slab_destroy(slab_desc* slab_desc_p, cache* cachep)
{
	// deinit all the objects
	// munmap memory
}