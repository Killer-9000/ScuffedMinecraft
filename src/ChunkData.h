#pragma once

#include <cstdint>
#include <list>

#include "ChunkPos.h"

struct ChunkData
{
#pragma pack(push,1)
	struct CompressedBlockID
	{
		uint16_t size;
		uint16_t blockId;
	};
#pragma pack(pop)
	//uint16_t* pallet;
	//Bitset blockIdxs;

	uint16_t* blockIDs;
	std::list<CompressedBlockID> compressedBlockIds;
	bool compressed = false;

	bool generated = false;

	ChunkData();
	~ChunkData();

	void Decompress();
	void Compress();

	inline static int GetIndex(int x, int y, int z);
	inline static int GetIndex(ChunkPos localBlockPos);

	uint16_t GetBlock(ChunkPos blockPos);
	uint16_t GetBlock(int x, int y, int z);
	uint16_t GetBlock(int index);
	void SetBlock(ChunkPos pos, uint16_t block);
	void SetBlock(int x, int y, int z, uint16_t block);
	void SetBlock(int index, uint16_t block);
};