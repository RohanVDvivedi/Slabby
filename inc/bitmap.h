#ifndef BITMAP_H
#define BITMAP_H

#include<stdio.h>
#include<stdint.h>
#include<string.h>

int get_bit(uint8_t* bitmap, uint32_t index);

void set_bit(uint8_t* bitmap, uint32_t index);

void reset_bit(uint8_t* bitmap, uint32_t index);

void set_all_bits(uint8_t* bitmap, uint32_t size);

void reset_all_bits(uint8_t* bitmap, uint32_t size);

void print_bitmap(uint8_t* bitmap, uint32_t size);

uint32_t bitmap_size_in_bytes(uint32_t size);

uint32_t find_first_set(uint8_t* bitmap, uint32_t start_index, uint32_t size);

#endif