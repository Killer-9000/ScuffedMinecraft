#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <queue>
#include <glm/glm.hpp>
#include <thread>
#include <mutex>

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

	struct ChunkRenderingData
	{
		GLuint vertexArray = 0;
		GLuint vertexBuffer = 0;
		GLuint elementBuffer = 0;
	};

	Planet(Shader* solidShader, Shader* waterShader, Shader* billboardShader);
	~Planet();

	ChunkData* GetChunkData(ChunkPos chunkPos);
	void Update(glm::vec3 cameraPos);

	Chunk* GetChunk(ChunkPos chunkPos);
	void ClearChunkQueue();

private:
	void ChunkThreadGenerator(int threadId);
	void AddChunkToGenerate(ChunkPos chunkPos);

// Variables
public:
	static Planet* planet;
	unsigned int numChunks = 0, numChunksRendered = 0;
	int renderDistance = 16;
	int renderHeight = 3;

	GLuint opaqueVAO = 0, billboardVAO = 0, transparentVAO = 0;

	int camChunkX = -100, camChunkY = -100, camChunkZ = -100;

private:
	std::unordered_map<ChunkPos, Chunk*, ChunkPosHash> chunks;
	std::unordered_map<ChunkPos, ChunkData*, ChunkPosHash> chunkData;
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
	std::queue<Chunk*> generatorChunks;
	std::mutex generatorMutex;

	bool shouldEnd = false;
};