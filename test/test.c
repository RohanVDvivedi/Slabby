#include<cache.h>

#include<stdint.h>
#include<string.h>
#include<alloca.h>

size_t number_of_objects_per_slab(cache* cachep);

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

void deinit(void* obj, size_t object_size)
{
	pthread_mutex_destroy(&(((object*)obj)->lock));
}

#define SLAB_SIZE (4096 * 1)

#define TEST_ALLOCS 80

// uncomment the below line to enable boundless memory allocation
#define WITH_MAX_MEMORY_LIMIT


// MEMORY_LIMIT to use will be decided on the basis of WITH_MAX_MEMORY_LIMIT
#ifdef WITH_MAX_MEMORY_LIMIT
	#define MEMORY_LIMIT (2 * 4096)
#else
	#define MEMORY_LIMIT NO_MAX_MEMORY_LIMIT
#endif

int main()
{
	printf("cache structure size : %zu\n\n", sizeof(cache));
	//printf("slab  structure size : %zu\n\n\n", sizeof(slab_desc));

	cache cash;

	cache_create(&cash, SLAB_SIZE, sizeof(object), MEMORY_LIMIT, init, deinit);

	cash.slab_size = 4096;
	cash.object_size = sizeof(object);

	printf("slab_size : %zu, object_size : %zu, objects_per_slab %zu\n", cash.slab_size, cash.object_size, number_of_objects_per_slab(&cash));

	uint32_t objects_n = TEST_ALLOCS;

	object** objects_allocated = alloca(objects_n * sizeof(object*));

	for(uint32_t i = 0; i < objects_n; i++)
	{
		objects_allocated[i] = cache_alloc(&cash);
		int counter = 0;
		if(objects_allocated[i])
		{
			objects_allocated[i]->counter++;
			counter = objects_allocated[i]->counter;
		}
		printf("\t%u \t alloc object addr : %p, object counter : %d\n", i, objects_allocated[i], counter);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		int fr = 0;
		if(objects_allocated[i])
			fr = cache_free(&cash, objects_allocated[i]);
		printf("free object %d => addr : %p\n", fr, objects_allocated[i]);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		objects_allocated[i] = cache_alloc(&cash);
		int counter = 0;
		if(objects_allocated[i])
		{
			objects_allocated[i]->counter++;
			counter = objects_allocated[i]->counter;
		}
		printf("\t%u \t alloc object addr : %p, object counter : %d\n", i, objects_allocated[i], counter);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		int fr = 0;
		if(objects_allocated[i])
			fr = cache_free(&cash, objects_allocated[i]);
		printf("free object %d => addr : %p\n", fr, objects_allocated[i]);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		objects_allocated[i] = cache_alloc(&cash);
		int counter = 0;
		if(objects_allocated[i])
		{
			objects_allocated[i]->counter++;
			counter = objects_allocated[i]->counter;
		}
		printf("\t%u \t alloc object addr : %p, object counter : %d\n", i, objects_allocated[i], counter);
	}

	printf("\n");

	for(uint32_t i = 0; i < objects_n; i++)
	{
		int fr = 0;
		if(objects_allocated[i])
			fr = cache_free(&cash, objects_allocated[i]);
		printf("free object %d => addr : %p\n", fr, objects_allocated[i]);
		if(i == objects_n / 5)
			break;
	}

	printf("\n");

	// testing double free
	for(uint32_t i = 0; i < objects_n; i++)
	{
		int fr = 0;
		if(objects_allocated[i])
			fr = cache_free(&cash, objects_allocated[i]);
		printf("free object %d => addr : %p\n", fr, objects_allocated[i]);
	}

	printf("\n");


	printf("cache destroyed with result : %d\n", cache_destroy(&cash));

	return 0;
}