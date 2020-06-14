#include<bitmap.h>

int get_bit(uint8_t* bitmap, uint32_t index)
{
	return (bitmap[index/8] >> (index % 8)) & 0x01;
}

void set_bit(uint8_t* bitmap, uint32_t index)
{
	bitmap[index/8] |= (1<<(index % 8));
}

void reset_bit(uint8_t* bitmap, uint32_t index)
{
	bitmap[index/8] &= ~(1<<(index % 8));
}

void set_all_bits(uint8_t* bitmap, uint32_t size)
{
	uint32_t bitmap_size = bitmap_size_in_bytes(size);
	for(uint32_t i = 0; i < bitmap_size; i++)
		bitmap[i] = 0xff;
}

void reset_all_bits(uint8_t* bitmap, uint32_t size)
{
	uint32_t bitmap_size = bitmap_size_in_bytes(size);
	for(uint32_t i = 0; i < bitmap_size; i++)
		bitmap[i] = 0;
}

void print_bitmap(uint8_t* bitmap, uint32_t size)
{
	for(uint32_t i = 0; i < size; i++)
	{
		if(i)
			printf(" ");
		printf("%d", get_bit(bitmap, i));
	}
	printf("\n");
}

uint32_t bitmap_size_in_bytes(uint32_t size)
{
	return (size/8) + ((size%8)?1:0);
}

uint32_t find_first_set(uint8_t* bitmap, uint32_t start_index, uint32_t size)
{
	uint32_t byte_index = start_index/8;
	uint32_t bytes_in_bitmap = bitmap_size_in_bytes(size);
	uint32_t bytes_to_check = bytes_in_bitmap;
	uint32_t bit_index = 0;
	while(bytes_to_check)
	{
		if(bitmap[byte_index])
		{
			bit_index = (byte_index*8) + (ffs(bitmap[byte_index]) - 1);
			if(bit_index < size)
				break;
		}
		byte_index = (byte_index + 1) % bytes_in_bitmap;
		bytes_to_check--;
	}
	return bit_index;
}