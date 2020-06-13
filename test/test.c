#include<cache.h>

#include<alloca.h>
#include<pthread.h>

typedef struct object object;
struct object
{
	// simple counter to count the number of recycles
	int counter;

	void* a;
	void* b;
	pthread_mutex_t lock;
	char ahoy[39];
	char bhoy[10];
};

void init(void* obj)
{
	((object*)obj)->counter = 0;
	((object*)obj)->a = NULL;
	((object*)obj)->b = NULL;
	pthread_mutex_init(&(((object*)obj)->lock), NULL);
	memset(((object*)obj)->ahoy, 0, 39);
	memset(((object*)obj)->bhoy, 0, 10);
}

void recycle(void* obj)
{
	((object*)obj)->counter++;
}

void deinit(void* obj)
{
	pthread_mutex_destroy(&(((object*)obj)->lock));
}

int main()
{
	printf("cache structure size : %lu\n", sizeof(cache));
	printf("slab  structure size : %lu\n\n\n", sizeof(slab_desc));

	cache cash;

	cash.slab_size = 4096;
	cash.object_size = sizeof(object);

	printf("slab_size : %lu, object_size : %lu\n", cash.slab_size, cash.object_size);

	printf("num_of_objects : %u\n\n", number_of_objects_per_slab(&cash));

	cash.init = init;
	cash.recycle = recycle;
	cash.deinit = deinit;

	slab_desc* slab_desc_p = slab_create(&cash);

	uint32_t objects_n = 4096/sizeof(object) - 3;
	object** objects_allocated = alloca(objects_n * sizeof(object*));

	for(uint32_t i = 0; i < objects_n; i++)
	{
		objects_allocated[i] = allocate_object(slab_desc_p, &cash);
		printf("alloc object addr : %p, object counter : %d\n", objects_allocated[i], objects_allocated[i]->counter);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		int fr = free_object(slab_desc_p, objects_allocated[i], &cash);
		printf("free object %d => addr : %p, object counter : %d\n", fr, objects_allocated[i], objects_allocated[i]->counter);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		objects_allocated[i] = allocate_object(slab_desc_p, &cash);
		printf("alloc object addr : %p, object counter : %d\n", objects_allocated[i], objects_allocated[i]->counter);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		int fr = free_object(slab_desc_p, objects_allocated[i], &cash);
		printf("free object %d => addr : %p, object counter : %d\n", fr, objects_allocated[i], objects_allocated[i]->counter);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		objects_allocated[i] = allocate_object(slab_desc_p, &cash);
		printf("alloc object addr : %p, object counter : %d\n", objects_allocated[i], objects_allocated[i]->counter);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		int fr = free_object(slab_desc_p, objects_allocated[i], &cash);
		printf("free object %d => addr : %p, object counter : %d\n", fr, objects_allocated[i], objects_allocated[i]->counter);
	}

	printf("\n");


	slab_destroy(slab_desc_p, &cash);

	return 0;
}