#pragma once

#include <glm/glm.hpp>
#include <mutex>
#include <thread>
#include <vector>

#include "graphics/Buffer.h"
#include "graphics/Shader.h"
#include "graphics/VertexArrayObject.h"
#include "ChunkPos.h"
#include "ChunkData.h"
#include "Vertex.h"

class Chunk : public std::enable_shared_from_this<Chunk>
{
public:
	typedef std::shared_ptr<Chunk> Ptr;

	Chunk(ChunkPos chunkPos, Shader* shader, Shader* waterShader);
	~Chunk();

	void GenerateChunkMesh(Chunk::Ptr left, Chunk::Ptr right, Chunk::Ptr front, Chunk::Ptr back);
	void PrepareRender();
	void Render(Shader* mainShader, Shader* billboardShader);
	void RenderWater(Shader* shader);
	uint16_t GetBlockAtPos(int x, int y, int z);
	void UpdateBlock(int x, int y, int z, uint16_t newBlock);
	void UpdateChunk();

public:
	bool surroundedChunks[4] = {false};
	std::mutex generatorMutex;
	ChunkData chunkData;
	ChunkPos chunkPos;
	bool ready;
	bool generated;
	bool markedForDelete;
	bool edgeUpdate;

	glm::vec3 worldPos;
	glm::mat4 modelMatrix;

	GeoBuffer::Node* opaqueTri = nullptr;
	GeoBuffer::Node* opaqueEle = nullptr;
	GeoBuffer::Node* billboardTri = nullptr;
	GeoBuffer::Node* billboardEle = nullptr;
	GeoBuffer::Node* waterTri = nullptr;
	GeoBuffer::Node* waterEle = nullptr;

private:
	std::vector<Vertex> mainVertices;
	std::vector<unsigned int> mainIndices;
	std::vector<Vertex> waterVertices;
	std::vector<unsigned int> waterIndices;
	std::vector<BillboardVertex> billboardVertices;
	std::vector<unsigned int> billboardIndices;

};
