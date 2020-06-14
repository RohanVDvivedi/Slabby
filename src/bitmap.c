#include<bitmap.h>

void set_bitmap(uint8_t* bitmap, uint32_t index)
{
	bitmap[index/8] |= (1<<(index % 8));
}

void reset_bitmap(uint8_t* bitmap, uint32_t index)
{
	bitmap[index/8] &= ~(1<<(index % 8));
}