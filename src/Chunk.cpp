#include "Chunk.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Planet.h"
#include "Blocks.h"
#include "WorldGen.h"

Chunk::Chunk(ChunkPos chunkPos, Shader* shader, Shader* waterShader)
	: chunkPos(chunkPos)
{
	worldPos = glm::vec3(chunkPos.x * (float)CHUNK_WIDTH, chunkPos.y * (float)CHUNK_HEIGHT, chunkPos.z * (float)CHUNK_WIDTH);

	ready = false;
	generated = false;
}

Chunk::~Chunk()
{
	glDeleteBuffers(1, &mainVBO);
	glDeleteBuffers(1, &mainEBO);
	glDeleteVertexArrays(1, &mainVAO);

	glDeleteBuffers(1, &waterVBO);
	glDeleteBuffers(1, &waterEBO);
	glDeleteVertexArrays(1, &waterVAO);

	glDeleteBuffers(1, &billboardVBO);
	glDeleteBuffers(1, &billboardEBO);
	glDeleteVertexArrays(1, &billboardVAO);
}

void Chunk::GenerateChunkMesh()
{
	mainVertices.clear();
	mainVertices.reserve(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH * 4 * 6);
	mainIndices.clear();
	mainIndices.reserve(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH * 6 * 6);

	waterVertices.clear();
	waterIndices.clear();
	billboardVertices.clear();
	billboardIndices.clear();

	numTrianglesMain = 0;
	numTrianglesWater = 0;
	numTrianglesBillboard = 0;

	unsigned int currentVertex = 0;
	unsigned int currentLiquidVertex = 0;
	unsigned int currentBillboardVertex = 0;
	for (char x = 0; x < CHUNK_WIDTH; x++)
	{
		for (char z = 0; z < CHUNK_WIDTH; z++)
		{
			for (char y = 0; y < CHUNK_HEIGHT; y++)
			{
				if (chunkData->GetBlock(x, y, z) == 0)
					continue;

				const Block* block = &Blocks::blocks[chunkData->GetBlock(x, y, z)];

				int topBlock;
				if (y < CHUNK_HEIGHT - 1)
				{
					topBlock = chunkData->GetBlock(x, y + 1, z);
				}
				else
				{
					int blockIndex = (x * CHUNK_WIDTH * CHUNK_WIDTH) + (z * CHUNK_HEIGHT) + 0;
					topBlock = upData->GetBlock(x, 0, z);
				}

				const Block* topBlockType = &Blocks::blocks[topBlock];

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
						northBlock = chunkData->GetBlock(x, y, z - 1);
					}
					else
					{
						northBlock = northData->GetBlock(x, y, CHUNK_WIDTH - 1);
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
						southBlock = chunkData->GetBlock(x, y, z + 1);
					}
					else
					{
						southBlock = southData->GetBlock(x, y, 0);
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
						westBlock = chunkData->GetBlock(x - 1, y, z);
					}
					else
					{
						westBlock = westData->GetBlock(CHUNK_WIDTH - 1, y, z);
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
						eastBlock = chunkData->GetBlock(x + 1, y, z);
					}
					else
					{
						eastBlock = eastData->GetBlock(0, y, z);
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
						bottomBlock = chunkData->GetBlock(x, y - 1, z);
					}
					else
					{
						//int blockIndex = x * chunkSize * chunkSize + z * chunkSize + (chunkSize - 1);
						bottomBlock = downData->GetBlock(x, CHUNK_HEIGHT - 1, z);
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

	mainVertices.shrink_to_fit();
	mainIndices.shrink_to_fit();

	//std::cout << "Finished generating in thread: " << std::this_thread::get_id() << '\n';

	generated = true;

	//std::cout << "Generated: " << generated << '\n';
}

void Chunk::PrepareRender()
{
	if (!ready && generated)
	{
		// Solid
		numTrianglesMain = mainIndices.size();

		glGenVertexArrays(1, &mainVAO);
		glGenBuffers(1, &mainVBO);
		glGenBuffers(1, &mainEBO);

		glBindVertexArray(mainVAO);

		glBindBuffer(GL_ARRAY_BUFFER, mainVBO);
		glBufferData(GL_ARRAY_BUFFER, mainVertices.size() * sizeof(Vertex), mainVertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mainEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mainIndices.size() * sizeof(unsigned int), mainIndices.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_BYTE, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, posX));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_BYTE, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texGridX));
		glEnableVertexAttribArray(1);
		glVertexAttribIPointer(2, 1, GL_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, direction));
		glEnableVertexAttribArray(2);

		// Water
		numTrianglesWater = waterIndices.size();

		glGenVertexArrays(1, &waterVAO);
		glGenBuffers(1, &waterVBO);
		glGenBuffers(1, &waterEBO);

		glBindVertexArray(waterVAO);

		glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
		glBufferData(GL_ARRAY_BUFFER, waterVertices.size() * sizeof(Vertex), waterVertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, waterIndices.size() * sizeof(unsigned int), waterIndices.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_BYTE, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, posX));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_BYTE, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texGridX));
		glEnableVertexAttribArray(1);
		glVertexAttribIPointer(2, 1, GL_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, direction));
		glEnableVertexAttribArray(2);
		ready = true;

		// Billboard
		numTrianglesBillboard = billboardIndices.size();

		glGenVertexArrays(1, &billboardVAO);
		glGenBuffers(1, &billboardVBO);
		glGenBuffers(1, &billboardEBO);

		glBindVertexArray(billboardVAO);

		glBindBuffer(GL_ARRAY_BUFFER, billboardVBO);
		glBufferData(GL_ARRAY_BUFFER, billboardVertices.size() * sizeof(BillboardVertex), billboardVertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, billboardEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, billboardIndices.size() * sizeof(unsigned int), billboardIndices.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BillboardVertex), (void*)offsetof(BillboardVertex, posX));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_BYTE, GL_FALSE, sizeof(BillboardVertex), (void*)offsetof(BillboardVertex, texGridX));
		glEnableVertexAttribArray(1);
		glVertexAttribIPointer(2, 1, GL_BYTE, sizeof(BillboardVertex), (void*)offsetof(BillboardVertex, direction));
		glEnableVertexAttribArray(2);

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

	// Render main mesh
	mainShader->use();
	mainShader->setMat4x4("model", modelMatrix);

	glBindVertexArray(mainVAO);
	glDrawElements(GL_TRIANGLES, numTrianglesMain, GL_UNSIGNED_INT, 0);

	// Render billboard mesh
	billboardShader->use();
	billboardShader->setMat4x4("model", modelMatrix);

	glDisable(GL_CULL_FACE);
	glBindVertexArray(billboardVAO);
	glDrawElements(GL_TRIANGLES, numTrianglesBillboard, GL_UNSIGNED_INT, 0);
	glEnable(GL_CULL_FACE);
}

void Chunk::RenderWater(Shader* shader)
{
	if (!ready)
		PrepareRender();

	//std::cout << "Rendering chunk " << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << '\n'
	//	<< "Chunk VAO: " << vertexArrayObject << '\n' << "Triangles: " << numTriangles << '\n';

	shader->setMat4x4("model", modelMatrix);

	glBindVertexArray(waterVAO);
	glDrawElements(GL_TRIANGLES, numTrianglesWater, GL_UNSIGNED_INT, 0);
}

uint16_t Chunk::GetBlockAtPos(int x, int y, int z)
{
	if (!ready)
		return 0;

	return chunkData->GetBlock(x, y, z);
}

void Chunk::UpdateBlock(int x, int y, int z, uint16_t newBlock)
{
	chunkData->SetBlock(x, y, z, newBlock);

	GenerateChunkMesh();

	// Main
	numTrianglesMain = mainIndices.size();

	glBindVertexArray(mainVAO);

	glBindBuffer(GL_ARRAY_BUFFER, mainVBO);
	glBufferData(GL_ARRAY_BUFFER, mainVertices.size() * sizeof(Vertex), mainVertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mainEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mainIndices.size() * sizeof(unsigned int), mainIndices.data(), GL_STATIC_DRAW);

	// Water
	numTrianglesWater = waterIndices.size();

	glBindVertexArray(waterVAO);

	glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
	glBufferData(GL_ARRAY_BUFFER, waterVertices.size() * sizeof(Vertex), waterVertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, waterIndices.size() * sizeof(unsigned int), waterIndices.data(), GL_STATIC_DRAW);

	// Billboard
	numTrianglesBillboard = billboardIndices.size();;

	glBindVertexArray(billboardVAO);

	glBindBuffer(GL_ARRAY_BUFFER, billboardVBO);
	glBufferData(GL_ARRAY_BUFFER, billboardVertices.size() * sizeof(Vertex), billboardVertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, billboardEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, billboardIndices.size() * sizeof(unsigned int), billboardIndices.data(), GL_STATIC_DRAW);


	if (x == 0)
	{
		Chunk* westChunk = Planet::planet->GetChunk({ chunkPos.x - 1, chunkPos.y, chunkPos.z });
		if (westChunk != nullptr)
			westChunk->UpdateChunk();
	}
	else if (x == CHUNK_WIDTH - 1)
	{
		Chunk* eastChunk = Planet::planet->GetChunk({ chunkPos.x + 1, chunkPos.y, chunkPos.z });
		if (eastChunk != nullptr)
			eastChunk->UpdateChunk();
	}

	if (y == 0)
	{
		Chunk* downChunk = Planet::planet->GetChunk({ chunkPos.x, chunkPos.y - 1, chunkPos.z });
		if (downChunk != nullptr)
			downChunk->UpdateChunk();
	}
	else if (y == CHUNK_HEIGHT - 1)
	{
		Chunk* upChunk = Planet::planet->GetChunk({ chunkPos.x, chunkPos.y + 1, chunkPos.z });
		if (upChunk != nullptr)
			upChunk->UpdateChunk();
	}

	if (z == 0)
	{
		Chunk* northChunk = Planet::planet->GetChunk({ chunkPos.x, chunkPos.y, chunkPos.z - 1 });
		if (northChunk != nullptr)
			northChunk->UpdateChunk();
	}
	else if (z == CHUNK_WIDTH - 1)
	{
		Chunk* southChunk = Planet::planet->GetChunk({ chunkPos.x, chunkPos.y, chunkPos.z + 1 });
		if (southChunk != nullptr)
			southChunk->UpdateChunk();
	}
}

void Chunk::UpdateChunk()
{
	GenerateChunkMesh();

	// Main
	numTrianglesMain = mainIndices.size();

	glBindVertexArray(mainVAO);

	glBindBuffer(GL_ARRAY_BUFFER, mainVBO);
	glBufferData(GL_ARRAY_BUFFER, mainVertices.size() * sizeof(Vertex), mainVertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mainEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mainIndices.size() * sizeof(unsigned int), mainIndices.data(), GL_STATIC_DRAW);

	// Water
	numTrianglesWater = waterIndices.size();

	glBindVertexArray(waterVAO);

	glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
	glBufferData(GL_ARRAY_BUFFER, waterVertices.size() * sizeof(Vertex), waterVertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, waterIndices.size() * sizeof(unsigned int), waterIndices.data(), GL_STATIC_DRAW);

	// Billboard
	numTrianglesBillboard = billboardIndices.size();

	glBindVertexArray(billboardVAO);

	glBindBuffer(GL_ARRAY_BUFFER, billboardVBO);
	glBufferData(GL_ARRAY_BUFFER, billboardVertices.size() * sizeof(Vertex), billboardVertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, billboardEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, billboardIndices.size() * sizeof(unsigned int), billboardIndices.data(), GL_STATIC_DRAW);
}