#ifndef SLAB_H
#define SLAB_H

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>

typedef struct slab slab;
struct slab
{
	slab* next;
	slab* prev;

	// pointer to the first object
	// number_of_objects_per_slab = (slab_size / object_size)
	// ith object => (i < number_of_objects_per_slab) ? (objects + (object_size * i)) : NULL;
	void* objects;

	// 1 bit is used to represent if an object is allocated or not
	// 1 means allocated
	// number_of_objects_per_slab = (slab_size / object_size)
	// (size in bytes) length of bit map = number_of_objects_per_slab / 8
	// to check if the ith object is occupied  => (allocation_bit_map[i/8]&(1<<(i%8)))
	uint8_t allocation_bit_map[];
};

void slab_create(slab* slabp, void* objects, size_t objects_per_slab, void (*init)(void*));

void slab_destroy(slab* slabp, void (*deinit)(void*));

#endif