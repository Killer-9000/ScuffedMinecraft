#pragma once

#include <cstdint>

#include "Bitset.h"
#include "ChunkPos.h"

struct ChunkData
{
	//uint16_t* pallet;
	//Bitset blockIdxs;

	uint16_t* blockIDs;

	bool generated = false;

	ChunkData();
	~ChunkData();

	inline int GetIndex(int x, int y, int z) const;
	inline int GetIndex(ChunkPos localBlockPos) const;

	uint16_t GetBlock(ChunkPos blockPos);
	uint16_t GetBlock(int x, int y, int z);
	void SetBlock(int x, int y, int z, uint16_t block);
};