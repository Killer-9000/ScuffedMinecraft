#pragma once

#include <assert.h>
#include <malloc.h>
#include <math.h>
#include <stdint.h>

#include <xtr1common>

class Bitset
{
public:
	Bitset() { }
	Bitset(size_t bitcount)
	{
		m_bitcount = bitcount;
		m_data = (uint8_t*)calloc((size_t)ceilf((float)(m_bitcount + 56)/ 8), 1);
	}

	~Bitset()
	{

	}

	uint64_t Get(size_t pos, size_t length)
	{
		assert(sizeof(uint64_t) >= length && "Datatype isn't large enough to contain bit value.");
		assert(pos + length <= m_bitcount && "Reading out-of-bounds.");
		assert(m_data && "Attempting to read from unallocated data");

		uint64_t value = *(uint64_t*)(m_data + (size_t)floorf((float)pos / 8));

		uint64_t mask = ~0;
		mask = mask << (64 - length);
		mask = mask >> (64 - length - pos % 64);

		value = (value & mask);
		value = value >> pos % 64;

		return value;
	}

	void Set(uint64_t data, size_t pos, size_t length)
	{
		assert(sizeof(uint64_t) >= length && "Datatype isn't large enough to contain bit value.");
		assert(pos + length <= m_bitcount && "Writing out-of-bounds.");
		assert(m_data && "Attempting to write to unallocated data");

		uint64_t* value = (uint64_t*)(m_data + (size_t)floorf((float)pos / 8));

		uint64_t mask = ~0;
		mask = mask << (64 - length);
		mask = mask >> (64 - length - pos % 64);

		data = data << pos % 64;
		data = (data & mask);

		*value = (*value & ~mask);
		*value = (*value | data);

	}

private:
	size_t m_bitcount = 0;
	uint8_t* m_data = nullptr;
};