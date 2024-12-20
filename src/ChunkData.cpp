#include "ChunkData.h"

#include <tracy/Tracy.hpp>
#include "Planet.h"

ChunkData::ChunkData()
{
	ZoneScoped;
	blockIDs = new uint16_t[CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH];
}

ChunkData::~ChunkData()
{
	ZoneScoped;
	delete[] blockIDs;
}

__forceinline int ChunkData::GetIndex(int x, int y, int z) const
{
	return (y * CHUNK_WIDTH * CHUNK_WIDTH) + (x) + (z * CHUNK_WIDTH);

	return (x * CHUNK_WIDTH * CHUNK_WIDTH) + (z * CHUNK_HEIGHT) + y;
}

__forceinline int ChunkData::GetIndex(ChunkPos localBlockPos) const
{
	return GetIndex(localBlockPos.x, localBlockPos.y, localBlockPos.z);
}

uint16_t ChunkData::GetBlock(ChunkPos blockPos)
{
	return blockIDs[GetIndex(blockPos)];
}

uint16_t ChunkData::GetBlock(int x, int y, int z)
{
	return blockIDs[GetIndex(x, y, z)];
}

void ChunkData::SetBlock(int x, int y, int z, uint16_t block)
{
	blockIDs[GetIndex(x, y, z)] = block;
}