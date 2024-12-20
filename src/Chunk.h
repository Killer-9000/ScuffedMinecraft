#pragma once

#include <vector>
#include <thread>
#include <glm/glm.hpp>

#include "Shader.h"
#include "Vertex.h"
#include "ChunkPos.h"
#include "ChunkData.h"

class Chunk
{
public:
	Chunk(ChunkPos chunkPos, Shader* shader, Shader* waterShader);
	~Chunk();

	void GenerateChunkMesh();
	void PrepareRender();
	void Render(Shader* mainShader, Shader* billboardShader);
	void RenderWater(Shader* shader);
	uint16_t GetBlockAtPos(int x, int y, int z);
	void UpdateBlock(int x, int y, int z, uint16_t newBlock);
	void UpdateChunk();

public:
	ChunkData* chunkData;
	ChunkData* northData;
	ChunkData* southData;
	ChunkData* upData;
	ChunkData* downData;
	ChunkData* eastData;
	ChunkData* westData;
	ChunkPos chunkPos;
	bool ready;
	bool generated;

	glm::vec3 worldPos;
	glm::mat4 modelMatrix;

	unsigned int mainVAO, waterVAO, billboardVAO;
	unsigned int mainVBO, mainEBO, waterVBO, waterEBO, billboardVBO, billboardEBO;
	unsigned int numTrianglesMain, numTrianglesWater, numTrianglesBillboard;

private:
	std::vector<Vertex> mainVertices;
	std::vector<unsigned int> mainIndices;
	std::vector<Vertex> waterVertices;
	std::vector<unsigned int> waterIndices;
	std::vector<BillboardVertex> billboardVertices;
	std::vector<unsigned int> billboardIndices;

};
