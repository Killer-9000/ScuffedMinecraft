#include "ChunkData.h"

#include <tracy/Tracy.hpp>
#include "Planet.h"

ChunkData::ChunkData()
{
	ZoneScoped;
}

ChunkData::~ChunkData()
{
	ZoneScoped;
	delete[] blockIDs;
}

void ChunkData::Decompress()
{
	if (!compressed)
		return;
	ZoneScoped;

	blockIDs = new uint16_t[CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH];

	auto itr = compressedBlockIds.begin();
	for (int i = 0, j = 0; i < CHUNK_HEIGHT * CHUNK_WIDTH * CHUNK_WIDTH; i++)
	{
		if (i >= j + itr->size)
		{
			j += itr->size;
			itr++;
		}

		blockIDs[i] = itr->blockId;
	}

	compressedBlockIds.clear();
	compressed = false;
}

void ChunkData::Compress()
{
	if (compressed)
		return;
	ZoneScoped;

	//std::vector<CompressedBlockID> compressedSections;
	//CompressedBlockID compressedSection{UINT16_MAX,UINT16_MAX};
	compressedBlockIds.emplace_back(CompressedBlockID{ 1, blockIDs[0] });
	auto itr = compressedBlockIds.begin();
	for (int i = 1; i < CHUNK_HEIGHT * CHUNK_WIDTH * CHUNK_WIDTH; i++)
	{
		if (itr->size == UINT16_MAX || itr->blockId != blockIDs[i])
		{
			compressedBlockIds.emplace_back(CompressedBlockID{ 1, blockIDs[i] });
			itr++;
		}
		else
			itr->size++;
	}

	//compressedBlockIds.resize(); = new CompressedBlockID[compressedSections.size()];
	//for (int i = 0; i < compressedSections.size(); i++)
	//{
	//	compressedBlockIds[i].size = compressedSections[i].size;
	//	compressedBlockIds[i].blockId = compressedSections[i].blockId;
	//	if (i > 0)
	//		compressedBlockIds[i - 1].next = &compressedSections[i];
	//}

	delete[] blockIDs;
	blockIDs = nullptr;
	compressed = true;
}

__forceinline int ChunkData::GetIndex(int x, int y, int z)
{
	return (y * CHUNK_WIDTH * CHUNK_WIDTH) + (x) + (z * CHUNK_WIDTH);

	//return (x * CHUNK_WIDTH * CHUNK_WIDTH) + (z * CHUNK_HEIGHT) + y;
}

__forceinline int ChunkData::GetIndex(ChunkPos localBlockPos)
{
	return GetIndex(localBlockPos.x, localBlockPos.y, localBlockPos.z);
}

uint16_t ChunkData::GetBlock(ChunkPos blockPos)
{
	int index = GetIndex(blockPos);
	return GetBlock(index);
}

uint16_t ChunkData::GetBlock(int x, int y, int z)
{
	int index = GetIndex(x, y, z);
	return GetBlock(index);
}

uint16_t ChunkData::GetBlock(int index)
{
	if (compressed)
	{
		auto itr = compressedBlockIds.begin();
		int i = 0;
		while (i <= index && itr != compressedBlockIds.end())
		{
			if (i + itr->size > index)
			{
				return itr->blockId;
			}

			i += itr->size;
			itr++;
		}

		assert(false && "Failed to find block in compressed list???");
		return 0;
	}
	else
		return blockIDs[index];
}

void ChunkData::SetBlock(ChunkPos pos, uint16_t block)
{
	int index = GetIndex(pos);
	SetBlock(index, block);
}

void ChunkData::SetBlock(int x, int y, int z, uint16_t block)
{
	int index = GetIndex(x, y, z);
	SetBlock(index, block);
}

void ChunkData::SetBlock(int index, uint16_t block)
{
	if (compressed)
	{
		auto itr = compressedBlockIds.begin();
		int i = 0;
		while (i <= index)
		{
			if (i + itr->size > index)
			{
				auto itr2 = itr;
				itr2++;
				if (index == i + itr->size - 1 && itr2->blockId == block)
				{
					itr->size -= 1;
					itr2->size += 1;
					return;
				}
				else
				{
					uint16_t oldBlock = itr->blockId;
					uint16_t oldSize = itr->size;
					itr->size = (index - i) + 1;
					oldSize -= itr->size + 1;
					compressedBlockIds.emplace(++itr, CompressedBlockID{ 1, block });
					compressedBlockIds.emplace(++itr, CompressedBlockID{ oldSize, oldBlock });
					return;
				}
			}

			i += itr->size;
			itr++;
		}
	}
	else
		blockIDs[index] = block;
}
