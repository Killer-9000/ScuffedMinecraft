#include "Chunk.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
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
}

void Chunk::GenerateChunkMesh(Chunk* left, Chunk* right, Chunk* front, Chunk* back)
{
	ready = false;
	generated = false;
	ZoneScoped;

	mainVertices.clear();
	mainIndices.clear();
	waterVertices.clear();
	waterIndices.clear();
	billboardVertices.clear();
	billboardIndices.clear();

	if (true)
	{
		enum SIDES
		{
			NONE =   0x0,
			TOP =    0x1,
			BOTTOM = 0x2,
			WEST =   0x4,
			EAST =   0x8,
			NORTH =  0x10,
			SOUTH =  0x20,
			ALL =    0x3F
		};

		int* shownSides = new int[CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH];

		// Work out which sides are shown.
		for (int x = 0; x < CHUNK_WIDTH; x++)
		{
			for (int z = 0; z < CHUNK_WIDTH; z++)
			{
				for (int y = 0; y < CHUNK_HEIGHT; y++)
				{
					int blockIndex = ChunkData::GetIndex(x, y, z);
					const Block* block = &Blocks::blocks[chunkData.GetBlock(blockIndex)];

					if (block->blockType == Block::TRANSPARENT)
					{
						shownSides[blockIndex] = SIDES::NONE;
						continue;
					}

					if (block->blockType == Block::BILLBOARD)
					{
						shownSides[blockIndex] = SIDES::NONE;
						continue;
					}

					shownSides[blockIndex] = SIDES::NONE;

					int showSide = Block::TRANSPARENT | Block::LEAVES | Block::BILLBOARD | Block::LIQUID;

					Block::BLOCK_TYPE otherBlock;
					// North side //
					{
						if (z > 0)
						{
							otherBlock = Blocks::blocks[chunkData.GetBlock(x, y, z - 1)].blockType;
						}
						else
						{
							if (back && back->chunkData.generated)
								otherBlock = Blocks::blocks[back->chunkData.GetBlock(x, y, CHUNK_WIDTH - 1)].blockType;
							else
								otherBlock = Block::TRANSPARENT;
						}

						if (block->blockType == Block::LIQUID && otherBlock == Block::LIQUID)
							shownSides[blockIndex] |= 0;
						else
							shownSides[blockIndex] |= (otherBlock & showSide) == otherBlock ? SIDES::NORTH : 0;
					}

					// South side //
					{
						if (z < CHUNK_WIDTH - 1)
						{
							otherBlock = Blocks::blocks[chunkData.GetBlock(x, y, z + 1)].blockType;
						}
						else
						{
							if (front && front->chunkData.generated)
								otherBlock = Blocks::blocks[front->chunkData.GetBlock(x, y, 0)].blockType;
							else
								otherBlock = Block::TRANSPARENT;
						}

						if (block->blockType == Block::LIQUID && otherBlock == Block::LIQUID)
							shownSides[blockIndex] |= 0;
						else
							shownSides[blockIndex] |= (otherBlock & showSide) == otherBlock ? SIDES::SOUTH : 0;
					}

					// East side //
					{
						if (x > 0)
						{
							otherBlock = Blocks::blocks[chunkData.GetBlock(x - 1, y, z)].blockType;
						}
						else
						{
							if (left && left->chunkData.generated)
								otherBlock = Blocks::blocks[left->chunkData.GetBlock(CHUNK_WIDTH - 1, y, z)].blockType;
							else
								otherBlock = Block::TRANSPARENT;
						}

						if (block->blockType == Block::LIQUID && otherBlock == Block::LIQUID)
							shownSides[blockIndex] |= 0;
						else
							shownSides[blockIndex] |= (otherBlock & showSide) == otherBlock ? SIDES::EAST : 0;
					}

					// West side //
					{
						if (x < CHUNK_WIDTH - 1)
						{
							otherBlock = Blocks::blocks[chunkData.GetBlock(x + 1, y, z)].blockType;
						}
						else
						{
							if (right && right->chunkData.generated)
								otherBlock = Blocks::blocks[right->chunkData.GetBlock(0, y, z)].blockType;
							else
								otherBlock = Block::TRANSPARENT;
						}

						if (block->blockType == Block::LIQUID && otherBlock == Block::LIQUID)
							shownSides[blockIndex] |= 0;
						else
							shownSides[blockIndex] |= (otherBlock & showSide) == otherBlock ? SIDES::WEST : 0;
					}

					// Bottom side //
					{
						if (y > 0)
						{
							otherBlock = Blocks::blocks[chunkData.GetBlock(x, y - 1, z)].blockType;
						}
						else
						{
							otherBlock = Block::TRANSPARENT;
						}

						if (block->blockType == Block::LIQUID && otherBlock == Block::LIQUID)
							shownSides[blockIndex] |= 0;
						else
							shownSides[blockIndex] |= (otherBlock & showSide) == otherBlock ? SIDES::BOTTOM : 0;
					}

					// Top side //
					{
						if (y < CHUNK_HEIGHT - 1)
						{
							otherBlock = Blocks::blocks[chunkData.GetBlock(x, y + 1, z)].blockType;
						}
						else
						{
							otherBlock = Block::TRANSPARENT;
						}

						if (block->blockType == Block::LIQUID && otherBlock == Block::LIQUID)
							shownSides[blockIndex] |= 0;
						else
							shownSides[blockIndex] |= (otherBlock & showSide) == otherBlock ? SIDES::TOP : 0;
					}
				}
			}
		}

		uint32_t currentVertex = 0;
		uint32_t currentLiquidVertex = 0;
		uint32_t currentBillboardVertex = 0;

		// Collect shown sides into triangles.
		for (int x = 0; x < CHUNK_WIDTH; x++)
		{
			for (int z = 0; z < CHUNK_WIDTH; z++)
			{
				for (int y = 0; y < CHUNK_HEIGHT; y++)
				{
					int blockIndex = ChunkData::GetIndex(x, y, z);
					const Block* block = &Blocks::blocks[chunkData.GetBlock(blockIndex)];

					if (block->blockType == Block::BILLBOARD)
					{
						billboardVertices.push_back(BillboardVertex(x + .85355f, y + 0, z + .85355f, block->sideMinX, block->sideMinY));
						billboardVertices.push_back(BillboardVertex(x + .14645f, y + 0, z + .14645f, block->sideMaxX, block->sideMinY));
						billboardVertices.push_back(BillboardVertex(x + .85355f, y + 1, z + .85355f, block->sideMinX, block->sideMaxY));
						billboardVertices.push_back(BillboardVertex(x + .14645f, y + 1, z + .14645f, block->sideMaxX, block->sideMaxY));

						billboardIndices.push_back(currentBillboardVertex + 0);
						billboardIndices.push_back(currentBillboardVertex + 3);
						billboardIndices.push_back(currentBillboardVertex + 1);
						billboardIndices.push_back(currentBillboardVertex + 0);
						billboardIndices.push_back(currentBillboardVertex + 2);
						billboardIndices.push_back(currentBillboardVertex + 3);
						currentBillboardVertex += 4;

						billboardVertices.push_back(BillboardVertex(x + .14645f, y + 0, z + .85355f, block->sideMinX, block->sideMinY));
						billboardVertices.push_back(BillboardVertex(x + .85355f, y + 0, z + .14645f, block->sideMaxX, block->sideMinY));
						billboardVertices.push_back(BillboardVertex(x + .14645f, y + 1, z + .85355f, block->sideMinX, block->sideMaxY));
						billboardVertices.push_back(BillboardVertex(x + .85355f, y + 1, z + .14645f, block->sideMaxX, block->sideMaxY));

						billboardIndices.push_back(currentBillboardVertex + 0);
						billboardIndices.push_back(currentBillboardVertex + 3);
						billboardIndices.push_back(currentBillboardVertex + 1);
						billboardIndices.push_back(currentBillboardVertex + 0);
						billboardIndices.push_back(currentBillboardVertex + 2);
						billboardIndices.push_back(currentBillboardVertex + 3);
						currentBillboardVertex += 4;

						continue;
					}

					if ((shownSides[blockIndex] & SIDES::NORTH) == SIDES::NORTH)
					{
						if (block->blockType == Block::LIQUID)
						{
							waterVertices.push_back(Vertex(x + 1, y + 0, z + 0, block->sideMinX, block->sideMinY, 0));
							waterVertices.push_back(Vertex(x + 0, y + 0, z + 0, block->sideMaxX, block->sideMinY, 0));
							waterVertices.push_back(Vertex(x + 1, y + 1, z + 0, block->sideMinX, block->sideMaxY, 0));
							waterVertices.push_back(Vertex(x + 0, y + 1, z + 0, block->sideMaxX, block->sideMaxY, 0));

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
							mainVertices.push_back(Vertex(x + 1, y + 0, z + 0, block->sideMinX, block->sideMinY, 0));
							mainVertices.push_back(Vertex(x + 0, y + 0, z + 0, block->sideMaxX, block->sideMinY, 0));
							mainVertices.push_back(Vertex(x + 1, y + 1, z + 0, block->sideMinX, block->sideMaxY, 0));
							mainVertices.push_back(Vertex(x + 0, y + 1, z + 0, block->sideMaxX, block->sideMaxY, 0));

							mainIndices.push_back(currentVertex + 0);
							mainIndices.push_back(currentVertex + 3);
							mainIndices.push_back(currentVertex + 1);
							mainIndices.push_back(currentVertex + 0);
							mainIndices.push_back(currentVertex + 2);
							mainIndices.push_back(currentVertex + 3);
							currentVertex += 4;
						}
					}
					if ((shownSides[blockIndex] & SIDES::SOUTH) == SIDES::SOUTH)
					{
						if (block->blockType == Block::LIQUID)
						{
							waterVertices.push_back(Vertex(x + 0, y + 0, z + 1, block->sideMinX, block->sideMinY, 1));
							waterVertices.push_back(Vertex(x + 1, y + 0, z + 1, block->sideMaxX, block->sideMinY, 1));
							waterVertices.push_back(Vertex(x + 0, y + 1, z + 1, block->sideMinX, block->sideMaxY, 1));
							waterVertices.push_back(Vertex(x + 1, y + 1, z + 1, block->sideMaxX, block->sideMaxY, 1));

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
							mainVertices.push_back(Vertex(x + 0, y + 0, z + 1, block->sideMinX, block->sideMinY, 1));
							mainVertices.push_back(Vertex(x + 1, y + 0, z + 1, block->sideMaxX, block->sideMinY, 1));
							mainVertices.push_back(Vertex(x + 0, y + 1, z + 1, block->sideMinX, block->sideMaxY, 1));
							mainVertices.push_back(Vertex(x + 1, y + 1, z + 1, block->sideMaxX, block->sideMaxY, 1));

							mainIndices.push_back(currentVertex + 0);
							mainIndices.push_back(currentVertex + 3);
							mainIndices.push_back(currentVertex + 1);
							mainIndices.push_back(currentVertex + 0);
							mainIndices.push_back(currentVertex + 2);
							mainIndices.push_back(currentVertex + 3);
							currentVertex += 4;
						}
					}
					if ((shownSides[blockIndex] & SIDES::EAST) == SIDES::EAST)
					{
						if (block->blockType == Block::LIQUID)
						{
							waterVertices.push_back(Vertex(x + 0, y + 0, z + 0, block->sideMinX, block->sideMinY, 2));
							waterVertices.push_back(Vertex(x + 0, y + 0, z + 1, block->sideMaxX, block->sideMinY, 2));
							waterVertices.push_back(Vertex(x + 0, y + 1, z + 0, block->sideMinX, block->sideMaxY, 2));
							waterVertices.push_back(Vertex(x + 0, y + 1, z + 1, block->sideMaxX, block->sideMaxY, 2));

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
							mainVertices.push_back(Vertex(x + 0, y + 0, z + 0, block->sideMinX, block->sideMinY, 2));
							mainVertices.push_back(Vertex(x + 0, y + 0, z + 1, block->sideMaxX, block->sideMinY, 2));
							mainVertices.push_back(Vertex(x + 0, y + 1, z + 0, block->sideMinX, block->sideMaxY, 2));
							mainVertices.push_back(Vertex(x + 0, y + 1, z + 1, block->sideMaxX, block->sideMaxY, 2));

							mainIndices.push_back(currentVertex + 0);
							mainIndices.push_back(currentVertex + 3);
							mainIndices.push_back(currentVertex + 1);
							mainIndices.push_back(currentVertex + 0);
							mainIndices.push_back(currentVertex + 2);
							mainIndices.push_back(currentVertex + 3);
							currentVertex += 4;
						}
					}
					if ((shownSides[blockIndex] & SIDES::WEST) == SIDES::WEST)
					{
						if (block->blockType == Block::LIQUID)
						{
							waterVertices.push_back(Vertex(x + 1, y + 0, z + 1, block->sideMinX, block->sideMinY, 3));
							waterVertices.push_back(Vertex(x + 1, y + 0, z + 0, block->sideMaxX, block->sideMinY, 3));
							waterVertices.push_back(Vertex(x + 1, y + 1, z + 1, block->sideMinX, block->sideMaxY, 3));
							waterVertices.push_back(Vertex(x + 1, y + 1, z + 0, block->sideMaxX, block->sideMaxY, 3));

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
							mainVertices.push_back(Vertex(x + 1, y + 0, z + 1, block->sideMinX, block->sideMinY, 3));
							mainVertices.push_back(Vertex(x + 1, y + 0, z + 0, block->sideMaxX, block->sideMinY, 3));
							mainVertices.push_back(Vertex(x + 1, y + 1, z + 1, block->sideMinX, block->sideMaxY, 3));
							mainVertices.push_back(Vertex(x + 1, y + 1, z + 0, block->sideMaxX, block->sideMaxY, 3));

							mainIndices.push_back(currentVertex + 0);
							mainIndices.push_back(currentVertex + 3);
							mainIndices.push_back(currentVertex + 1);
							mainIndices.push_back(currentVertex + 0);
							mainIndices.push_back(currentVertex + 2);
							mainIndices.push_back(currentVertex + 3);
							currentVertex += 4;
						}
					}
					if ((shownSides[blockIndex] & SIDES::BOTTOM) == SIDES::BOTTOM)
					{
						if (block->blockType == Block::LIQUID)
						{
							waterVertices.push_back(Vertex(x + 1, y + 0, z + 1, block->bottomMinX, block->bottomMinY, 4));
							waterVertices.push_back(Vertex(x + 0, y + 0, z + 1, block->bottomMaxX, block->bottomMinY, 4));
							waterVertices.push_back(Vertex(x + 1, y + 0, z + 0, block->bottomMinX, block->bottomMaxY, 4));
							waterVertices.push_back(Vertex(x + 0, y + 0, z + 0, block->bottomMaxX, block->bottomMaxY, 4));

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
							mainVertices.push_back(Vertex(x + 1, y + 0, z + 1, block->bottomMinX, block->bottomMinY, 4));
							mainVertices.push_back(Vertex(x + 0, y + 0, z + 1, block->bottomMaxX, block->bottomMinY, 4));
							mainVertices.push_back(Vertex(x + 1, y + 0, z + 0, block->bottomMinX, block->bottomMaxY, 4));
							mainVertices.push_back(Vertex(x + 0, y + 0, z + 0, block->bottomMaxX, block->bottomMaxY, 4));

							mainIndices.push_back(currentVertex + 0);
							mainIndices.push_back(currentVertex + 3);
							mainIndices.push_back(currentVertex + 1);
							mainIndices.push_back(currentVertex + 0);
							mainIndices.push_back(currentVertex + 2);
							mainIndices.push_back(currentVertex + 3);
							currentVertex += 4;
						}
					}
					if ((shownSides[blockIndex] & SIDES::TOP) == SIDES::TOP)
					{
						if (block->blockType == Block::LIQUID)
						{
							//waterVertices.push_back(Vertex(x + 0, y + 1, z + 1, block->topMinX, block->topMinY, 5));
							//waterVertices.push_back(Vertex(x + 1, y + 1, z + 1, block->topMaxX, block->topMinY, 5));
							//waterVertices.push_back(Vertex(x + 0, y + 1, z + 0, block->topMinX, block->topMaxY, 5));
							//waterVertices.push_back(Vertex(x + 1, y + 1, z + 0, block->topMaxX, block->topMaxY, 5));
							//
							//waterIndices.push_back(currentLiquidVertex + 0);
							//waterIndices.push_back(currentLiquidVertex + 3);
							//waterIndices.push_back(currentLiquidVertex + 1);
							//waterIndices.push_back(currentLiquidVertex + 0);
							//waterIndices.push_back(currentLiquidVertex + 2);
							//waterIndices.push_back(currentLiquidVertex + 3);
							//currentLiquidVertex += 4;

							waterVertices.push_back(Vertex(x + 1, y + 1, z + 1, block->topMinX, block->topMinY, 5));
							waterVertices.push_back(Vertex(x + 0, y + 1, z + 1, block->topMaxX, block->topMinY, 5));
							waterVertices.push_back(Vertex(x + 1, y + 1, z + 0, block->topMinX, block->topMaxY, 5));
							waterVertices.push_back(Vertex(x + 0, y + 1, z + 0, block->topMaxX, block->topMaxY, 5));

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
							mainVertices.push_back(Vertex(x + 0, y + 1, z + 1, block->topMinX, block->topMinY, 5));
							mainVertices.push_back(Vertex(x + 1, y + 1, z + 1, block->topMaxX, block->topMinY, 5));
							mainVertices.push_back(Vertex(x + 0, y + 1, z + 0, block->topMinX, block->topMaxY, 5));
							mainVertices.push_back(Vertex(x + 1, y + 1, z + 0, block->topMaxX, block->topMaxY, 5));

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

		delete[] shownSides;
	}
	else
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
						billboardVertices.push_back(BillboardVertex(x + .85355f, y + 0, z + .85355f, block->sideMinX, block->sideMinY));
						billboardVertices.push_back(BillboardVertex(x + .14645f, y + 0, z + .14645f, block->sideMaxX, block->sideMinY));
						billboardVertices.push_back(BillboardVertex(x + .85355f, y + 1, z + .85355f, block->sideMinX, block->sideMaxY));
						billboardVertices.push_back(BillboardVertex(x + .14645f, y + 1, z + .14645f, block->sideMaxX, block->sideMaxY));

						billboardIndices.push_back(currentBillboardVertex + 0);
						billboardIndices.push_back(currentBillboardVertex + 3);
						billboardIndices.push_back(currentBillboardVertex + 1);
						billboardIndices.push_back(currentBillboardVertex + 0);
						billboardIndices.push_back(currentBillboardVertex + 2);
						billboardIndices.push_back(currentBillboardVertex + 3);
						currentBillboardVertex += 4;

						billboardVertices.push_back(BillboardVertex(x + .14645f, y + 0, z + .85355f, block->sideMinX, block->sideMinY));
						billboardVertices.push_back(BillboardVertex(x + .85355f, y + 0, z + .14645f, block->sideMaxX, block->sideMinY));
						billboardVertices.push_back(BillboardVertex(x + .14645f, y + 1, z + .85355f, block->sideMinX, block->sideMaxY));
						billboardVertices.push_back(BillboardVertex(x + .85355f, y + 1, z + .14645f, block->sideMaxX, block->sideMaxY));

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
							if (front && front->chunkData.generated)
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
								waterVertices.push_back(Vertex(x + 1, y + 0, z + 0, block->sideMinX, block->sideMinY, 0));
								waterVertices.push_back(Vertex(x + 0, y + 0, z + 0, block->sideMaxX, block->sideMinY, 0));
								waterVertices.push_back(Vertex(x + 1, y + 1, z + 0, block->sideMinX, block->sideMaxY, 0));
								waterVertices.push_back(Vertex(x + 0, y + 1, z + 0, block->sideMaxX, block->sideMaxY, 0));

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
								mainVertices.push_back(Vertex(x + 1, y + 0, z + 0, block->sideMinX, block->sideMinY, 0));
								mainVertices.push_back(Vertex(x + 0, y + 0, z + 0, block->sideMaxX, block->sideMinY, 0));
								mainVertices.push_back(Vertex(x + 1, y + 1, z + 0, block->sideMinX, block->sideMaxY, 0));
								mainVertices.push_back(Vertex(x + 0, y + 1, z + 0, block->sideMaxX, block->sideMaxY, 0));

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
							if (back && back->chunkData.generated)
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
								waterVertices.push_back(Vertex(x + 0, y + 0, z + 1, block->sideMinX, block->sideMinY, 1));
								waterVertices.push_back(Vertex(x + 1, y + 0, z + 1, block->sideMaxX, block->sideMinY, 1));
								waterVertices.push_back(Vertex(x + 0, y + 1, z + 1, block->sideMinX, block->sideMaxY, 1));
								waterVertices.push_back(Vertex(x + 1, y + 1, z + 1, block->sideMaxX, block->sideMaxY, 1));

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
								mainVertices.push_back(Vertex(x + 0, y + 0, z + 1, block->sideMinX, block->sideMinY, 1));
								mainVertices.push_back(Vertex(x + 1, y + 0, z + 1, block->sideMaxX, block->sideMinY, 1));
								mainVertices.push_back(Vertex(x + 0, y + 1, z + 1, block->sideMinX, block->sideMaxY, 1));
								mainVertices.push_back(Vertex(x + 1, y + 1, z + 1, block->sideMaxX, block->sideMaxY, 1));

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
							if (left && left->chunkData.generated)
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
								waterVertices.push_back(Vertex(x + 0, y + 0, z + 0, block->sideMinX, block->sideMinY, 2));
								waterVertices.push_back(Vertex(x + 0, y + 0, z + 1, block->sideMaxX, block->sideMinY, 2));
								waterVertices.push_back(Vertex(x + 0, y + 1, z + 0, block->sideMinX, block->sideMaxY, 2));
								waterVertices.push_back(Vertex(x + 0, y + 1, z + 1, block->sideMaxX, block->sideMaxY, 2));

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
								mainVertices.push_back(Vertex(x + 0, y + 0, z + 0, block->sideMinX, block->sideMinY, 2));
								mainVertices.push_back(Vertex(x + 0, y + 0, z + 1, block->sideMaxX, block->sideMinY, 2));
								mainVertices.push_back(Vertex(x + 0, y + 1, z + 0, block->sideMinX, block->sideMaxY, 2));
								mainVertices.push_back(Vertex(x + 0, y + 1, z + 1, block->sideMaxX, block->sideMaxY, 2));

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
							if (right && right->chunkData.generated)
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
								waterVertices.push_back(Vertex(x + 1, y + 0, z + 1, block->sideMinX, block->sideMinY, 3));
								waterVertices.push_back(Vertex(x + 1, y + 0, z + 0, block->sideMaxX, block->sideMinY, 3));
								waterVertices.push_back(Vertex(x + 1, y + 1, z + 1, block->sideMinX, block->sideMaxY, 3));
								waterVertices.push_back(Vertex(x + 1, y + 1, z + 0, block->sideMaxX, block->sideMaxY, 3));

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
								mainVertices.push_back(Vertex(x + 1, y + 0, z + 1, block->sideMinX, block->sideMinY, 3));
								mainVertices.push_back(Vertex(x + 1, y + 0, z + 0, block->sideMaxX, block->sideMinY, 3));
								mainVertices.push_back(Vertex(x + 1, y + 1, z + 1, block->sideMinX, block->sideMaxY, 3));
								mainVertices.push_back(Vertex(x + 1, y + 1, z + 0, block->sideMaxX, block->sideMaxY, 3));

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
								waterVertices.push_back(Vertex(x + 1, y + 0, z + 1, block->bottomMinX, block->bottomMinY, 4));
								waterVertices.push_back(Vertex(x + 0, y + 0, z + 1, block->bottomMaxX, block->bottomMinY, 4));
								waterVertices.push_back(Vertex(x + 1, y + 0, z + 0, block->bottomMinX, block->bottomMaxY, 4));
								waterVertices.push_back(Vertex(x + 0, y + 0, z + 0, block->bottomMaxX, block->bottomMaxY, 4));

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
								mainVertices.push_back(Vertex(x + 1, y + 0, z + 1, block->bottomMinX, block->bottomMinY, 4));
								mainVertices.push_back(Vertex(x + 0, y + 0, z + 1, block->bottomMaxX, block->bottomMinY, 4));
								mainVertices.push_back(Vertex(x + 1, y + 0, z + 0, block->bottomMinX, block->bottomMaxY, 4));
								mainVertices.push_back(Vertex(x + 0, y + 0, z + 0, block->bottomMaxX, block->bottomMaxY, 4));

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
								waterVertices.push_back(Vertex(x + 0, y + 1, z + 1, block->topMinX, block->topMinY, 5));
								waterVertices.push_back(Vertex(x + 1, y + 1, z + 1, block->topMaxX, block->topMinY, 5));
								waterVertices.push_back(Vertex(x + 0, y + 1, z + 0, block->topMinX, block->topMaxY, 5));
								waterVertices.push_back(Vertex(x + 1, y + 1, z + 0, block->topMaxX, block->topMaxY, 5));

								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 3);
								waterIndices.push_back(currentLiquidVertex + 1);
								waterIndices.push_back(currentLiquidVertex + 0);
								waterIndices.push_back(currentLiquidVertex + 2);
								waterIndices.push_back(currentLiquidVertex + 3);
								currentLiquidVertex += 4;

								waterVertices.push_back(Vertex(x + 1, y + 1, z + 1, block->topMinX, block->topMinY, 5));
								waterVertices.push_back(Vertex(x + 0, y + 1, z + 1, block->topMaxX, block->topMinY, 5));
								waterVertices.push_back(Vertex(x + 1, y + 1, z + 0, block->topMinX, block->topMaxY, 5));
								waterVertices.push_back(Vertex(x + 0, y + 1, z + 0, block->topMaxX, block->topMaxY, 5));

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
							mainVertices.push_back(Vertex(x + 0, y + 1, z + 1, block->topMinX, block->topMinY, 5));
							mainVertices.push_back(Vertex(x + 1, y + 1, z + 1, block->topMaxX, block->topMinY, 5));
							mainVertices.push_back(Vertex(x + 0, y + 1, z + 0, block->topMinX, block->topMaxY, 5));
							mainVertices.push_back(Vertex(x + 1, y + 1, z + 0, block->topMaxX, block->topMaxY, 5));

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
	generated = true;
}

void Chunk::PrepareRender()
{
	if (!ready && generated && !markedForDelete)
	{
		opaqueTri = nullptr;
		opaqueEle = nullptr;
		if (mainIndices.size())
		{
			Planet::DrawingData& data = Planet::planet->opaqueDrawingData;

			opaqueTri = data.vbo.AddData(mainVertices.size() * sizeof(Vertex), mainVertices.data());
			opaqueEle = data.ebo.AddData(mainIndices.size() * sizeof(uint32_t), mainIndices.data());

			mainVertices.clear();
			mainVertices.shrink_to_fit();
			mainIndices.clear();
			mainIndices.shrink_to_fit();
		}

		billboardTri = nullptr;
		billboardEle = nullptr;
		if (billboardIndices.size())
		{
			Planet::DrawingData& data = Planet::planet->billboardDrawingData;

			billboardTri = data.vbo.AddData(billboardVertices.size() * sizeof(BillboardVertex), billboardVertices.data());
			billboardEle = data.ebo.AddData(billboardIndices.size() * sizeof(uint32_t), billboardIndices.data());
			billboardVertices.clear();
			billboardVertices.shrink_to_fit();
			billboardIndices.clear();
			billboardIndices.shrink_to_fit();
		}

		waterTri = nullptr;
		waterEle = nullptr;
		if (waterIndices.size())
		{
			Planet::DrawingData& data = Planet::planet->transparentDrawingData;

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

	Planet::planet->AddChunkToGenerate(chunkPos);

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