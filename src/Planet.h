#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <glm/glm.hpp>
#include <concurrentqueue.h>

#include "ChunkPos.h"
#include "ChunkData.h"
#include "Chunk.h"
#include "ChunkPosHash.h"

constexpr unsigned int CHUNK_WIDTH = 32; // x/z
constexpr unsigned int CHUNK_HEIGHT = 256; // y

class Planet
{
// Methods
public:
	struct DrawingData
	{
		VertexArrayObject vao;
		GeoBuffer vbo = GeoBuffer(GL_ARRAY_BUFFER);
		GeoBuffer ebo = GeoBuffer(GL_ELEMENT_ARRAY_BUFFER);
		Buffer ibo = Buffer(GL_DRAW_INDIRECT_BUFFER);
	};

	Planet(Shader* solidShader, Shader* waterShader, Shader* billboardShader);
	~Planet();

	void AddChunkToGenerate(ChunkPos chunkPos);
	ChunkData* GetChunkData(ChunkPos chunkPos);
	void Update(glm::vec3 cameraPos);

	Chunk* GetChunk(ChunkPos chunkPos);
	void ClearChunkQueue()
	{
		clearChunkQueue = 1;
	}
	void UpdateChunkQueue()
	{
		lastCamX++;
	}

private:
	void ChunkThreadGenerator(int threadId);

// Variables
public:
	static Planet* planet;
	unsigned int numChunks = 0, numChunksRendered = 0;
	int renderDistance = 1;
	int renderHeight = 3;
	int clearChunkQueue = 0;
	bool deleteChunks = true;
	bool loadChunks = true;

	std::mutex chunkDeletionMutex;
	std::shared_mutex chunkBlockMutex;
	std::mutex chunkMeshMutex;

	DrawingData opaqueDrawingData;
	DrawingData billboardDrawingData;
	DrawingData transparentDrawingData;

	int camChunkX = -100, camChunkY = -100, camChunkZ = -100;

private:
	std::unordered_map<ChunkPos, Chunk*, ChunkPosHash> chunks;
	//std::unordered_map<ChunkPos, ChunkData*, ChunkPosHash> chunkData;
	std::queue<ChunkPos> chunkQueue;
	std::queue<ChunkPos> chunkDataQueue;
	std::queue<ChunkPos> chunkDataDeleteQueue;
	unsigned int chunksLoading = 0;
	int lastCamX = -100, lastCamY = -100, lastCamZ = -100;

	Shader* solidShader;
	Shader* waterShader;
	Shader* billboardShader;

	std::mutex chunkMutex;

	std::vector<std::thread> generatorThreads;
	moodycamel::ConcurrentQueue<Chunk*> generatorChunks;

	bool shouldEnd = false;
};