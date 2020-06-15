#include<cache.h>

#include<string.h>
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

void init(void* obj, size_t object_size)
{
	((object*)obj)->counter = 0;
	((object*)obj)->a = NULL;
	((object*)obj)->b = NULL;
	pthread_mutex_init(&(((object*)obj)->lock), NULL);
	memset(((object*)obj)->ahoy, 0, 39);
	memset(((object*)obj)->bhoy, 0, 10);
}

void recycle(void* obj, size_t object_size)
{
	((object*)obj)->counter++;
}

void deinit(void* obj, size_t object_size)
{
	pthread_mutex_destroy(&(((object*)obj)->lock));
}

#define SLAB_SIZE (4096 * 1)

#define TEST_ALLOCS 100

int main()
{
	printf("cache structure size : %lu\n", sizeof(cache));
	printf("slab  structure size : %lu\n\n\n", sizeof(slab_desc));

	cache cash;

	cache_create(&cash, SLAB_SIZE, sizeof(object), init, recycle, deinit);

	cash.slab_size = 4096;
	cash.object_size = sizeof(object);

	printf("slab_size : %lu, object_size : %lu\n", cash.slab_size, cash.object_size);

	printf("num_of_objects : %u\n\n", cash.objects_per_slab);

	uint32_t objects_n = TEST_ALLOCS;

	object** objects_allocated = alloca(objects_n * sizeof(object*));

	for(uint32_t i = 0; i < objects_n; i++)
	{
		objects_allocated[i] = cache_alloc(&cash);
		printf("alloc object addr : %p, object counter : %d\n", objects_allocated[i], objects_allocated[i]->counter);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		int fr = cache_free(&cash, objects_allocated[i]);
		printf("free object %d => addr : %p\n", fr, objects_allocated[i]);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		objects_allocated[i] = cache_alloc(&cash);
		printf("alloc object addr : %p, object counter : %d\n", objects_allocated[i], objects_allocated[i]->counter);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		int fr = cache_free(&cash, objects_allocated[i]);
		printf("free object %d => addr : %p\n", fr, objects_allocated[i]);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		objects_allocated[i] = cache_alloc(&cash);
		printf("alloc object addr : %p, object counter : %d\n", objects_allocated[i], objects_allocated[i]->counter);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		int fr = cache_free(&cash, objects_allocated[i]);
		printf("free object %d => addr : %p\n", fr, objects_allocated[i]);
	}

	printf("\n");


	printf("cache destroyed with result : %d\n", cache_destroy(&cash));

	return 0;
}