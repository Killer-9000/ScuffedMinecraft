#include "Chunk.h"

#include <bitset>
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/printf.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tracy/Tracy.hpp>

#include "Planet.h"
#include "Blocks.h"
#include "WorldGen.h"

Chunk::Chunk(ChunkPos chunkPos, Shader* shader, Shader* waterShader)
	: chunkPos(chunkPos)
{
	worldPos = glm::vec3(chunkPos.x * (float)CHUNK_WIDTH, chunkPos.y * (float)CHUNK_HEIGHT, chunkPos.z * (float)CHUNK_WIDTH);

	ready = false;
	generated = false;
	markedForDelete = false;
	edgeUpdate = false;
}

Chunk::~Chunk()
{
	ZoneScoped;
	Planet::planet->opaqueDrawingData.vbo.RemoveData(opaqueTri);
	Planet::planet->opaqueDrawingData.ebo.RemoveData(opaqueEle);
	Planet::planet->billboardDrawingData.vbo.RemoveData(billboardTri);
	Planet::planet->billboardDrawingData.ebo.RemoveData(billboardEle);
	Planet::planet->transparentDrawingData.vbo.RemoveData(waterTri);
	Planet::planet->transparentDrawingData.ebo.RemoveData(waterEle);
}

void Chunk::GenerateChunkMesh(Chunk::Ptr left, Chunk::Ptr right, Chunk::Ptr front, Chunk::Ptr back)
{
	generated = false;
	ZoneScoped;
	ZoneNameF("Chunk::GenerateChunkMesh %i %i %i", chunkPos.x, chunkPos.y, chunkPos.z);

	bool leftGenerated = left && left->chunkData.generated;
	bool rightGenerated = right && right->chunkData.generated;
	bool frontGenerated = front && front->chunkData.generated;
	bool backGenerated = back && back->chunkData.generated;

	mainVertices.clear();
	mainIndices.clear();
	waterVertices.clear();
	waterIndices.clear();
	billboardVertices.clear();
	billboardIndices.clear();

	if (true)
	{
		enum
		{
			FRONT = 0,
			BACK,
			LEFT,
			RIGHT,
			TOP,
			BOTTOM
		};
		std::bitset<CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH * 6> exposedFaces;
		for (int x = 0; x < CHUNK_WIDTH; x++)
		{
			for (int z = 0; z < CHUNK_WIDTH; z++)
			{
				for (int y = 0; y < CHUNK_HEIGHT; y++)
				{
					int index = ChunkData::GetIndex(x, y, z);

					if (chunkData.GetBlock(index) == Blocks::AIR)
					{
						exposedFaces[index * 6 + FRONT] = 0;
						exposedFaces[index * 6 + BACK] = 0;
						exposedFaces[index * 6 + LEFT] = 0;
						exposedFaces[index * 6 + RIGHT] = 0;
						exposedFaces[index * 6 + TOP] = 0;
						exposedFaces[index * 6 + BOTTOM] = 0;

						continue;
					}

					exposedFaces[index * 6 + FRONT] = 1;
					exposedFaces[index * 6 + BACK] = 1;
					exposedFaces[index * 6 + LEFT] = 1;
					exposedFaces[index * 6 + RIGHT] = 1;
					exposedFaces[index * 6 + TOP] = 1;
					exposedFaces[index * 6 + BOTTOM] = 1;

					if (z == CHUNK_WIDTH - 1 || chunkData.GetBlock(x, y, z + 1) != Blocks::AIR)
						exposedFaces[index * 6 + FRONT] = 0;
					if (z == 0 || chunkData.GetBlock(x, y, z - 1) != Blocks::AIR)
						exposedFaces[index * 6 + BACK] = 0;
					if (x == 0 || chunkData.GetBlock(x - 1, y, z) != Blocks::AIR)
						exposedFaces[index * 6 + LEFT] = 0;
					if (x == CHUNK_WIDTH - 1 || chunkData.GetBlock(x + 1, y, z) != Blocks::AIR)
						exposedFaces[index * 6 + RIGHT] = 0;
					if (y == CHUNK_HEIGHT - 1 || chunkData.GetBlock(x, y + 1, z) != Blocks::AIR)
						exposedFaces[index * 6 + TOP] = 0;
					if (y == 0 || chunkData.GetBlock(x, y - 1, z) != Blocks::AIR)
						exposedFaces[index * 6 + BOTTOM] = 0;
				}
			}
		}

		mainVertices.reserve(10000);
		mainIndices.reserve(10000);
		waterVertices.reserve(10000);
		waterIndices.reserve(10000);
		billboardVertices.reserve(10000);
		billboardIndices.reserve(10000);

		uint32_t currentVertex = 0;

		for (int x = 0; x < CHUNK_WIDTH; x++)
		{
			for (int z = 0; z < CHUNK_WIDTH; z++)
			{
				for (int y = 0; y < CHUNK_HEIGHT; y++)
				{
					int index = ChunkData::GetIndex(x, y, z);
					const Block& block = Blocks::blocks[chunkData.GetBlock(index)];

					if (exposedFaces[index * 6 + FRONT])
					{
						mainVertices.push_back({ { x + 0, y + 0, z + 1 }, { block.sideMinX, block.sideMinY}, 1 });
						mainVertices.push_back({ { x + 1, y + 0, z + 1 }, { block.sideMaxX, block.sideMinY}, 1 });
						mainVertices.push_back({ { x + 0, y + 1, z + 1 }, { block.sideMinX, block.sideMaxY}, 1 });
						mainVertices.push_back({ { x + 1, y + 1, z + 1 }, { block.sideMaxX, block.sideMaxY}, 1 });

						mainIndices.push_back(currentVertex + 0);
						mainIndices.push_back(currentVertex + 3);
						mainIndices.push_back(currentVertex + 1);
						mainIndices.push_back(currentVertex + 0);
						mainIndices.push_back(currentVertex + 2);
						mainIndices.push_back(currentVertex + 3);
						currentVertex += 4;
					}
					if (exposedFaces[index * 6 + BACK])
					{
						mainVertices.push_back({ { x + 1, y + 0, z + 0 }, { block.sideMinX, block.sideMinY}, 0 });
						mainVertices.push_back({ { x + 0, y + 0, z + 0 }, { block.sideMaxX, block.sideMinY}, 0 });
						mainVertices.push_back({ { x + 1, y + 1, z + 0 }, { block.sideMinX, block.sideMaxY}, 0 });
						mainVertices.push_back({ { x + 0, y + 1, z + 0 }, { block.sideMaxX, block.sideMaxY}, 0 });

						mainIndices.push_back(currentVertex + 0);
						mainIndices.push_back(currentVertex + 3);
						mainIndices.push_back(currentVertex + 1);
						mainIndices.push_back(currentVertex + 0);
						mainIndices.push_back(currentVertex + 2);
						mainIndices.push_back(currentVertex + 3);
						currentVertex += 4;
					}
					if (exposedFaces[index * 6 + LEFT])
					{
						mainVertices.push_back({ { x + 0, y + 0, z + 0 }, { block.sideMinX, block.sideMinY}, 2 });
						mainVertices.push_back({ { x + 0, y + 0, z + 1 }, { block.sideMaxX, block.sideMinY}, 2 });
						mainVertices.push_back({ { x + 0, y + 1, z + 0 }, { block.sideMinX, block.sideMaxY}, 2 });
						mainVertices.push_back({ { x + 0, y + 1, z + 1 }, { block.sideMaxX, block.sideMaxY}, 2 });

						mainIndices.push_back(currentVertex + 0);
						mainIndices.push_back(currentVertex + 3);
						mainIndices.push_back(currentVertex + 1);
						mainIndices.push_back(currentVertex + 0);
						mainIndices.push_back(currentVertex + 2);
						mainIndices.push_back(currentVertex + 3);
						currentVertex += 4;
					}
					if (exposedFaces[index * 6 + RIGHT])
					{
						mainVertices.push_back({ { x + 1, y + 0, z + 1 }, { block.sideMinX, block.sideMinY}, 3 });
						mainVertices.push_back({ { x + 1, y + 0, z + 0 }, { block.sideMaxX, block.sideMinY}, 3 });
						mainVertices.push_back({ { x + 1, y + 1, z + 1 }, { block.sideMinX, block.sideMaxY}, 3 });
						mainVertices.push_back({ { x + 1, y + 1, z + 0 }, { block.sideMaxX, block.sideMaxY}, 3 });

						mainIndices.push_back(currentVertex + 0);
						mainIndices.push_back(currentVertex + 3);
						mainIndices.push_back(currentVertex + 1);
						mainIndices.push_back(currentVertex + 0);
						mainIndices.push_back(currentVertex + 2);
						mainIndices.push_back(currentVertex + 3);
						currentVertex += 4;
					}
					if (exposedFaces[index * 6 + TOP])
					{
						mainVertices.push_back({ { x + 0, y + 1, z + 1 }, { block.topMinX, block.topMinY}, 5 });
						mainVertices.push_back({ { x + 1, y + 1, z + 1 }, { block.topMaxX, block.topMinY}, 5 });
						mainVertices.push_back({ { x + 0, y + 1, z + 0 }, { block.topMinX, block.topMaxY}, 5 });
						mainVertices.push_back({ { x + 1, y + 1, z + 0 }, { block.topMaxX, block.topMaxY}, 5 });

						mainIndices.push_back(currentVertex + 0);
						mainIndices.push_back(currentVertex + 3);
						mainIndices.push_back(currentVertex + 1);
						mainIndices.push_back(currentVertex + 0);
						mainIndices.push_back(currentVertex + 2);
						mainIndices.push_back(currentVertex + 3);
						currentVertex += 4;
					}
					if (exposedFaces[index * 6 + BOTTOM])
					{
						mainVertices.push_back({ { x + 1, y + 0, z + 1 }, { block.bottomMinX, block.bottomMinY}, 4 });
						mainVertices.push_back({ { x + 0, y + 0, z + 1 }, { block.bottomMaxX, block.bottomMinY}, 4 });
						mainVertices.push_back({ { x + 1, y + 0, z + 0 }, { block.bottomMinX, block.bottomMaxY}, 4 });
						mainVertices.push_back({ { x + 0, y + 0, z + 0 }, { block.bottomMaxX, block.bottomMaxY}, 4 });

						mainIndices.push_back(currentVertex + 0);
						mainIndices.push_back(currentVertex + 3);
						mainIndices.push_back(currentVertex + 1);
						mainIndices.push_back(currentVertex + 0);
						mainIndices.push_back(currentVertex + 2);
						mainIndices.push_back(currentVertex + 3);
						currentVertex += 4;
					}
				}
			}
		}
	}
	else if (true)
	{
		//mainVertices.reserve(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH * 4 * 6);
		//mainIndices.reserve(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH * 6 * 6);
		//waterVertices.reserve(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH * 4 * 6);
		//waterIndices.reserve(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH * 6 * 6);
		//billboardVertices.reserve(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH * 4 * 6);
		//billboardIndices.reserve(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH * 6 * 6);

		uint32_t currentVertex = 0;
		uint32_t currentLiquidVertex = 0;
		uint32_t currentBillboardVertex = 0;
		for (int x = 0; x < CHUNK_WIDTH; x++)
		{
			for (int z = 0; z < CHUNK_WIDTH; z++)
			{
				for (int y = 0; y < CHUNK_HEIGHT; y++)
				{
					if (chunkData.GetBlock(x, y, z) == Blocks::AIR)
						continue;

					const Block* block = &Blocks::blocks[chunkData.GetBlock(x, y, z)];

					if (block->blockType == Block::BILLBOARD)
					{
						billboardVertices.push_back({ { x + .85355f, y + 0, z + .85355f }, { block->sideMinX, block->sideMinY } });
						billboardVertices.push_back({ { x + .14645f, y + 0, z + .14645f }, { block->sideMaxX, block->sideMinY } });
						billboardVertices.push_back({ { x + .85355f, y + 1, z + .85355f }, { block->sideMinX, block->sideMaxY } });
						billboardVertices.push_back({ { x + .14645f, y + 1, z + .14645f }, { block->sideMaxX, block->sideMaxY } });

						billboardIndices.push_back(currentBillboardVertex + 0);
						billboardIndices.push_back(currentBillboardVertex + 3);
						billboardIndices.push_back(currentBillboardVertex + 1);
						billboardIndices.push_back(currentBillboardVertex + 0);
						billboardIndices.push_back(currentBillboardVertex + 2);
						billboardIndices.push_back(currentBillboardVertex + 3);
						currentBillboardVertex += 4;

						billboardVertices.push_back({ { x + .14645f, y + 0, z + .85355f }, { block->sideMinX, block->sideMinY } });
						billboardVertices.push_back({ { x + .85355f, y + 0, z + .14645f }, { block->sideMaxX, block->sideMinY } });
						billboardVertices.push_back({ { x + .14645f, y + 1, z + .85355f }, { block->sideMinX, block->sideMaxY } });
						billboardVertices.push_back({ { x + .85355f, y + 1, z + .14645f }, { block->sideMaxX, block->sideMaxY } });

						billboardIndices.push_back(currentBillboardVertex + 0);
						billboardIndices.push_back(currentBillboardVertex + 3);
						billboardIndices.push_back(currentBillboardVertex + 1);
						billboardIndices.push_back(currentBillboardVertex + 0);
						billboardIndices.push_back(currentBillboardVertex + 2);
						billboardIndices.push_back(currentBillboardVertex + 3);
						currentBillboardVertex += 4;

						continue;
					}

					// North
					{
						int northBlock;
						if (z > 0)
						{
							northBlock = chunkData.GetBlock(x, y, z - 1);
						}
						else
						{
							if (frontGenerated)
								northBlock = front->chunkData.GetBlock(x, y, CHUNK_WIDTH - 1);
							else
								northBlock = Blocks::AIR;
						}

						const Block* northBlockType = &Blocks::blocks[northBlock];

						if (northBlockType->blockType == Block::LEAVES
							|| northBlockType->blockType == Block::TRANSPARENT
							|| northBlockType->blockType == Block::BILLBOARD
							|| (northBlockType->blockType == Block::LIQUID && block->blockType != Block::LIQUID))
						{
							if (block->blockType == Block::LIQUID)
							{
								waterVertices.push_back({ { x + 1, y + 0, z + 0 }, { block->sideMinX, block->sideMinY}, 0 });
								waterVertices.push_back({ { x + 0, y + 0, z + 0 }, { block->sideMaxX, block->sideMinY}, 0 });
								waterVertices.push_back({ { x + 1, y + 1, z + 0 }, { block->sideMinX, block->sideMaxY}, 0 });
								waterVertices.push_back({ { x + 0, y + 1, z + 0 }, { block->sideMaxX, block->sideMaxY}, 0 });

								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 3);
								waterIndices.push_back(currentLiquidVertex + 1);
								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 2);
								waterIndices.push_back(currentLiquidVertex + 3);
								currentLiquidVertex += 4;
							}
							else
							{
								mainVertices.push_back({ { x + 1, y + 0, z + 0 }, { block->sideMinX, block->sideMinY}, 0 });
								mainVertices.push_back({ { x + 0, y + 0, z + 0 }, { block->sideMaxX, block->sideMinY}, 0 });
								mainVertices.push_back({ { x + 1, y + 1, z + 0 }, { block->sideMinX, block->sideMaxY}, 0 });
								mainVertices.push_back({ { x + 0, y + 1, z + 0 }, { block->sideMaxX, block->sideMaxY}, 0 });

								mainIndices.push_back(currentVertex + 0);
								mainIndices.push_back(currentVertex + 3);
								mainIndices.push_back(currentVertex + 1);
								mainIndices.push_back(currentVertex + 0);
								mainIndices.push_back(currentVertex + 2);
								mainIndices.push_back(currentVertex + 3);
								currentVertex += 4;
							}
						}
					}

					// South
					{
						int southBlock;
						if (z < CHUNK_WIDTH - 1)
						{
							southBlock = chunkData.GetBlock(x, y, z + 1);
						}
						else
						{
							if (backGenerated)
								southBlock = back->chunkData.GetBlock(x, y, 0);
							else
								southBlock = Blocks::AIR;
						}

						const Block* southBlockType = &Blocks::blocks[southBlock];

						if (southBlockType->blockType == Block::LEAVES
							|| southBlockType->blockType == Block::TRANSPARENT
							|| southBlockType->blockType == Block::BILLBOARD
							|| (southBlockType->blockType == Block::LIQUID && block->blockType != Block::LIQUID))
						{
							if (block->blockType == Block::LIQUID)
							{
								waterVertices.push_back({ { x + 0, y + 0, z + 1 }, { block->sideMinX, block->sideMinY}, 1 });
								waterVertices.push_back({ { x + 1, y + 0, z + 1 }, { block->sideMaxX, block->sideMinY}, 1 });
								waterVertices.push_back({ { x + 0, y + 1, z + 1 }, { block->sideMinX, block->sideMaxY}, 1 });
								waterVertices.push_back({ { x + 1, y + 1, z + 1 }, { block->sideMaxX, block->sideMaxY}, 1 });

								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 3);
								waterIndices.push_back(currentLiquidVertex + 1);
								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 2);
								waterIndices.push_back(currentLiquidVertex + 3);
								currentLiquidVertex += 4;
							}
							else
							{
								mainVertices.push_back({ { x + 0, y + 0, z + 1 }, { block->sideMinX, block->sideMinY}, 1 });
								mainVertices.push_back({ { x + 1, y + 0, z + 1 }, { block->sideMaxX, block->sideMinY}, 1 });
								mainVertices.push_back({ { x + 0, y + 1, z + 1 }, { block->sideMinX, block->sideMaxY}, 1 });
								mainVertices.push_back({ { x + 1, y + 1, z + 1 }, { block->sideMaxX, block->sideMaxY}, 1 });

								mainIndices.push_back(currentVertex + 0);
								mainIndices.push_back(currentVertex + 3);
								mainIndices.push_back(currentVertex + 1);
								mainIndices.push_back(currentVertex + 0);
								mainIndices.push_back(currentVertex + 2);
								mainIndices.push_back(currentVertex + 3);
								currentVertex += 4;
							}
						}
					}

					// West
					{
						int westBlock;
						if (x > 0)
						{
							westBlock = chunkData.GetBlock(x - 1, y, z);
						}
						else
						{
							if (leftGenerated)
								westBlock = left->chunkData.GetBlock(CHUNK_WIDTH - 1, y, z);
							else
								westBlock = Blocks::AIR;
						}

						const Block* westBlockType = &Blocks::blocks[westBlock];

						if (westBlockType->blockType == Block::LEAVES
							|| westBlockType->blockType == Block::TRANSPARENT
							|| westBlockType->blockType == Block::BILLBOARD
							|| (westBlockType->blockType == Block::LIQUID && block->blockType != Block::LIQUID))
						{
							if (block->blockType == Block::LIQUID)
							{
								waterVertices.push_back({ { x + 0, y + 0, z + 0 }, { block->sideMinX, block->sideMinY}, 2 });
								waterVertices.push_back({ { x + 0, y + 0, z + 1 }, { block->sideMaxX, block->sideMinY}, 2 });
								waterVertices.push_back({ { x + 0, y + 1, z + 0 }, { block->sideMinX, block->sideMaxY}, 2 });
								waterVertices.push_back({ { x + 0, y + 1, z + 1 }, { block->sideMaxX, block->sideMaxY}, 2 });

								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 3);
								waterIndices.push_back(currentLiquidVertex + 1);
								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 2);
								waterIndices.push_back(currentLiquidVertex + 3);
								currentLiquidVertex += 4;
							}
							else
							{
								mainVertices.push_back({ { x + 0, y + 0, z + 0 }, { block->sideMinX, block->sideMinY}, 2 });
								mainVertices.push_back({ { x + 0, y + 0, z + 1 }, { block->sideMaxX, block->sideMinY}, 2 });
								mainVertices.push_back({ { x + 0, y + 1, z + 0 }, { block->sideMinX, block->sideMaxY}, 2 });
								mainVertices.push_back({ { x + 0, y + 1, z + 1 }, { block->sideMaxX, block->sideMaxY}, 2 });

								mainIndices.push_back(currentVertex + 0);
								mainIndices.push_back(currentVertex + 3);
								mainIndices.push_back(currentVertex + 1);
								mainIndices.push_back(currentVertex + 0);
								mainIndices.push_back(currentVertex + 2);
								mainIndices.push_back(currentVertex + 3);
								currentVertex += 4;
							}
						}
					}

					// East
					{
						int eastBlock;
						if (x < CHUNK_WIDTH - 1)
						{
							eastBlock = chunkData.GetBlock(x + 1, y, z);
						}
						else
						{
							if (rightGenerated)
								eastBlock = right->chunkData.GetBlock(0, y, z);
							else
								eastBlock = Blocks::AIR;
						}

						const Block* eastBlockType = &Blocks::blocks[eastBlock];

						if (eastBlockType->blockType == Block::LEAVES
							|| eastBlockType->blockType == Block::TRANSPARENT
							|| eastBlockType->blockType == Block::BILLBOARD
							|| (eastBlockType->blockType == Block::LIQUID && block->blockType != Block::LIQUID))
						{
							if (block->blockType == Block::LIQUID)
							{
								waterVertices.push_back({ { x + 1, y + 0, z + 1 }, { block->sideMinX, block->sideMinY}, 3 });
								waterVertices.push_back({ { x + 1, y + 0, z + 0 }, { block->sideMaxX, block->sideMinY}, 3 });
								waterVertices.push_back({ { x + 1, y + 1, z + 1 }, { block->sideMinX, block->sideMaxY}, 3 });
								waterVertices.push_back({ { x + 1, y + 1, z + 0 }, { block->sideMaxX, block->sideMaxY}, 3 });

								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 3);
								waterIndices.push_back(currentLiquidVertex + 1);
								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 2);
								waterIndices.push_back(currentLiquidVertex + 3);
								currentLiquidVertex += 4;
							}
							else
							{
								mainVertices.push_back({ { x + 1, y + 0, z + 1 }, { block->sideMinX, block->sideMinY}, 3 });
								mainVertices.push_back({ { x + 1, y + 0, z + 0 }, { block->sideMaxX, block->sideMinY}, 3 });
								mainVertices.push_back({ { x + 1, y + 1, z + 1 }, { block->sideMinX, block->sideMaxY}, 3 });
								mainVertices.push_back({ { x + 1, y + 1, z + 0 }, { block->sideMaxX, block->sideMaxY}, 3 });

								mainIndices.push_back(currentVertex + 0);
								mainIndices.push_back(currentVertex + 3);
								mainIndices.push_back(currentVertex + 1);
								mainIndices.push_back(currentVertex + 0);
								mainIndices.push_back(currentVertex + 2);
								mainIndices.push_back(currentVertex + 3);
								currentVertex += 4;
							}
						}
					}

					// Bottom
					{
						int bottomBlock;
						if (y > 0)
						{
							bottomBlock = chunkData.GetBlock(x, y - 1, z);
						}
						else
						{
							//int blockIndex = x * chunkSize * chunkSize + z * chunkSize + (chunkSize - 1);
							bottomBlock = Blocks::AIR;
						}

						const Block* bottomBlockType = &Blocks::blocks[bottomBlock];

						if (bottomBlockType->blockType == Block::LEAVES
							|| bottomBlockType->blockType == Block::TRANSPARENT
							|| bottomBlockType->blockType == Block::BILLBOARD
							|| (bottomBlockType->blockType == Block::LIQUID && block->blockType != Block::LIQUID))
						{
							if (block->blockType == Block::LIQUID)
							{
								waterVertices.push_back({ { x + 1, y + 0, z + 1 }, { block->bottomMinX, block->bottomMinY}, 4 });
								waterVertices.push_back({ { x + 0, y + 0, z + 1 }, { block->bottomMaxX, block->bottomMinY}, 4 });
								waterVertices.push_back({ { x + 1, y + 0, z + 0 }, { block->bottomMinX, block->bottomMaxY}, 4 });
								waterVertices.push_back({ { x + 0, y + 0, z + 0 }, { block->bottomMaxX, block->bottomMaxY}, 4 });

								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 3);
								waterIndices.push_back(currentLiquidVertex + 1);
								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 2);
								waterIndices.push_back(currentLiquidVertex + 3);
								currentLiquidVertex += 4;
							}
							else
							{
								mainVertices.push_back({ { x + 1, y + 0, z + 1 }, { block->bottomMinX, block->bottomMinY}, 4 });
								mainVertices.push_back({ { x + 0, y + 0, z + 1 }, { block->bottomMaxX, block->bottomMinY}, 4 });
								mainVertices.push_back({ { x + 1, y + 0, z + 0 }, { block->bottomMinX, block->bottomMaxY}, 4 });
								mainVertices.push_back({ { x + 0, y + 0, z + 0 }, { block->bottomMaxX, block->bottomMaxY}, 4 });

								mainIndices.push_back(currentVertex + 0);
								mainIndices.push_back(currentVertex + 3);
								mainIndices.push_back(currentVertex + 1);
								mainIndices.push_back(currentVertex + 0);
								mainIndices.push_back(currentVertex + 2);
								mainIndices.push_back(currentVertex + 3);
								currentVertex += 4;
							}
						}
					}

					// Top
					{
						int topBlock;
						if (y < CHUNK_HEIGHT - 1)
						{
							topBlock = chunkData.GetBlock(x, y + 1, z);
						}
						else
						{
							//int blockIndex = (x * CHUNK_WIDTH * CHUNK_WIDTH) + (z * CHUNK_HEIGHT) + 0;
							topBlock = Blocks::AIR;
						}

						const Block* topBlockType = &Blocks::blocks[topBlock];

						if (block->blockType == Block::LIQUID)
						{
							if (topBlockType->blockType != Block::LIQUID)
							{
								waterVertices.push_back({ { x + 0, y + 1, z + 1 }, { block->topMinX, block->topMinY}, 5 });
								waterVertices.push_back({ { x + 1, y + 1, z + 1 }, { block->topMaxX, block->topMinY}, 5 });
								waterVertices.push_back({ { x + 0, y + 1, z + 0 }, { block->topMinX, block->topMaxY}, 5 });
								waterVertices.push_back({ { x + 1, y + 1, z + 0 }, { block->topMaxX, block->topMaxY}, 5 });

								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 3);
								waterIndices.push_back(currentLiquidVertex + 1);
								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 2);
								waterIndices.push_back(currentLiquidVertex + 3);
								currentLiquidVertex += 4;

								waterVertices.push_back({ { x + 1, y + 1, z + 1 }, { block->topMinX, block->topMinY}, 5 });
								waterVertices.push_back({ { x + 0, y + 1, z + 1 }, { block->topMaxX, block->topMinY}, 5 });
								waterVertices.push_back({ { x + 1, y + 1, z + 0 }, { block->topMinX, block->topMaxY}, 5 });
								waterVertices.push_back({ { x + 0, y + 1, z + 0 }, { block->topMaxX, block->topMaxY}, 5 });

								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 3);
								waterIndices.push_back(currentLiquidVertex + 1);
								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 2);
								waterIndices.push_back(currentLiquidVertex + 3);
								currentLiquidVertex += 4;
							}
						}
						else if (topBlockType->blockType == Block::LEAVES
							|| topBlockType->blockType == Block::TRANSPARENT
							|| topBlockType->blockType == Block::BILLBOARD
							|| topBlockType->blockType == Block::LIQUID)
						{
							mainVertices.push_back({ { x + 0, y + 1, z + 1 }, { block->topMinX, block->topMinY}, 5 });
							mainVertices.push_back({ { x + 1, y + 1, z + 1 }, { block->topMaxX, block->topMinY}, 5 });
							mainVertices.push_back({ { x + 0, y + 1, z + 0 }, { block->topMinX, block->topMaxY}, 5 });
							mainVertices.push_back({ { x + 1, y + 1, z + 0 }, { block->topMaxX, block->topMaxY}, 5 });

							mainIndices.push_back(currentVertex + 0);
							mainIndices.push_back(currentVertex + 3);
							mainIndices.push_back(currentVertex + 1);
							mainIndices.push_back(currentVertex + 0);
							mainIndices.push_back(currentVertex + 2);
							mainIndices.push_back(currentVertex + 3);
							currentVertex += 4;
						}
					}
				}
			}
		}
	}

	//std::cout << "Finished generating in thread: " << std::this_thread::get_id() << '\n';

	//std::cout << "Generated: " << generated << '\n';
	ready = false;
	generated = true;
}

void Chunk::PrepareRender()
{
	if (!ready && generated && !markedForDelete)
	{
		if (mainIndices.size())
		{
			Planet::DrawingData& data = Planet::planet->opaqueDrawingData;

			data.vbo.RemoveData(opaqueTri);
			data.ebo.RemoveData(opaqueEle);

			opaqueTri = data.vbo.AddData(mainVertices.size() * sizeof(Vertex), mainVertices.data());
			opaqueEle = data.ebo.AddData(mainIndices.size() * sizeof(uint32_t), mainIndices.data());

			mainVertices.clear();
			mainVertices.shrink_to_fit();
			mainIndices.clear();
			mainIndices.shrink_to_fit();
		}

		if (billboardIndices.size())
		{
			Planet::DrawingData& data = Planet::planet->billboardDrawingData;

			data.vbo.RemoveData(billboardTri);
			data.ebo.RemoveData(billboardEle);

			billboardTri = data.vbo.AddData(billboardVertices.size() * sizeof(BillboardVertex), billboardVertices.data());
			billboardEle = data.ebo.AddData(billboardIndices.size() * sizeof(uint32_t), billboardIndices.data());

			billboardVertices.clear();
			billboardVertices.shrink_to_fit();
			billboardIndices.clear();
			billboardIndices.shrink_to_fit();
		}

		if (waterIndices.size())
		{
			Planet::DrawingData& data = Planet::planet->transparentDrawingData;

			data.vbo.RemoveData(waterTri);
			data.ebo.RemoveData(waterEle);

			waterTri = data.vbo.AddData(waterVertices.size() * sizeof(Vertex), waterVertices.data());
			waterEle = data.ebo.AddData(waterIndices.size() * sizeof(uint32_t), waterIndices.data());

			waterVertices.clear();
			waterVertices.shrink_to_fit();
			waterIndices.clear();
			waterIndices.shrink_to_fit();
		}

		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, worldPos);

		ready = true;
	}
}

void Chunk::Render(Shader* mainShader, Shader* billboardShader)
{
	if (!ready)
		PrepareRender();

	//std::cout << "Rendering chunk " << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << '\n'
	//	<< "Chunk VAO: " << vertexArrayObject << '\n' << "Triangles: " << numTriangles << '\n';

	//// Render main mesh
	//mainShader->use();
	//mainShader->setMat4x4("model", modelMatrix);

	//glBindVertexArray(mainVAO);
	//glDrawElements(GL_TRIANGLES, numTrianglesMain, GL_UNSIGNED_INT, 0);

	//// Render billboard mesh
	//billboardShader->use();
	//billboardShader->setMat4x4("model", modelMatrix);

	//glDisable(GL_CULL_FACE);
	//glBindVertexArray(billboardVAO);
	//glDrawElements(GL_TRIANGLES, numTrianglesBillboard, GL_UNSIGNED_INT, 0);
	//glEnable(GL_CULL_FACE);
}

void Chunk::RenderWater(Shader* shader)
{
	if (!ready)
		PrepareRender();

	//std::cout << "Rendering chunk " << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << '\n'
	//	<< "Chunk VAO: " << vertexArrayObject << '\n' << "Triangles: " << numTriangles << '\n';

	//shader->setMat4x4("model", modelMatrix);

	//glBindVertexArray(waterVAO);
	//glDrawElements(GL_TRIANGLES, numTrianglesWater, GL_UNSIGNED_INT, 0);
}

uint16_t Chunk::GetBlockAtPos(int x, int y, int z)
{
	if (!ready)
		return 0;

	return chunkData.GetBlock(x, y, z);
}

void Chunk::UpdateBlock(int x, int y, int z, uint16_t newBlock)
{
	ZoneScoped;

	chunkData.SetBlock(x, y, z, newBlock);

	if (x == 0 || x == CHUNK_WIDTH - 1 || z == 0 || z == CHUNK_WIDTH - 1)
		edgeUpdate = true;

	UpdateChunk();
}

void Chunk::UpdateChunk()
{
	ZoneScoped;

	Planet::planet->AddChunkToGenerate(shared_from_this());

	//GenerateChunkMesh();

	// Main
	//numTrianglesMain = mainIndices.size();
	//mainVBO.SetData(mainVertices.size() * sizeof(Vertex), mainVertices.data());
	//mainVBO.SetData(mainIndices.size() * sizeof(uint32_t), mainIndices.data());

	//numTrianglesWater = waterIndices.size();
	//waterVBO.SetData(waterVertices.size() * sizeof(Vertex), waterVertices.data());
	//waterVBO.SetData(waterIndices.size() * sizeof(uint32_t), waterIndices.data());

	//numTrianglesBillboard = billboardIndices.size();
	//billboardVBO.SetData(billboardVertices.size() * sizeof(BillboardVertex), billboardVertices.data());
	//billboardVBO.SetData(billboardIndices.size() * sizeof(uint32_t), billboardIndices.data());
}