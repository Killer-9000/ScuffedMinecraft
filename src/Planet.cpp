#include "Planet.h"

#include <fmt/printf.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include <tracy/Tracy.hpp>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include "WorldGen.h"
#include "ChunkRenderer.h"

Planet* Planet::planet = nullptr;

//static const unsigned int CHUNK_SIZE = 32;

// Public
Planet::Planet(Shader* solidShader, Shader* waterShader, Shader* billboardShader)
	: solidShader(solidShader), waterShader(waterShader), billboardShader(billboardShader)
{
	uint32_t threads = std::thread::hardware_concurrency();
	threads /= 2;
	for (int i = 0; i < threads; i++)
		generatorThreads.emplace_back(&Planet::ChunkThreadGenerator, this, i);
}

Planet::~Planet()
{
	shouldEnd = true;
	for (auto& thread : generatorThreads)
		thread.join();
}

void Planet::Update(glm::vec3 cameraPos)
{
	ZoneScoped;
	camChunkX = cameraPos.x < 0 ? floor(cameraPos.x / CHUNK_WIDTH) : cameraPos.x / CHUNK_WIDTH;
	camChunkY = cameraPos.y < 0 ? floor(cameraPos.y / CHUNK_HEIGHT) : cameraPos.y / CHUNK_HEIGHT;
	camChunkZ = cameraPos.z < 0 ? floor(cameraPos.z / CHUNK_WIDTH) : cameraPos.z / CHUNK_WIDTH;

	//chunkMutex.lock();
	numChunks = chunks.size();
	ChunkRenderer::RenderOpaque(chunks, solidShader, billboardShader, chunksLoading, numChunksRendered);
	ChunkRenderer::RenderTransparent(chunks, waterShader);
	//chunkMutex.unlock();

	// Check if camera moved to new chunk
	if (camChunkX != lastCamX || camChunkY != lastCamY || camChunkZ != lastCamZ)
	{
		ZoneScopedN("Moved");
		// Player moved chunks, start new chunk queue
		lastCamX = camChunkX;
		lastCamY = camChunkY;
		lastCamZ = camChunkZ;

		//chunkMutex.lock();
		//chunkQueue = {};
		{
			ZoneScopedN("Add new chunks");
			if (chunks.find({ camChunkX, 0, camChunkZ }) == chunks.end())
				AddChunkToGenerate({ camChunkX, 0, camChunkZ });

			// Starting from the center
			for (int x = -(renderDistance / 2); x < renderDistance / 2; x++)
			{
				for (int z = -(renderDistance / 2); z < renderDistance / 2; z++)
				{
					float dist = sqrt(pow(abs(x), 2) + pow(abs(z), 2));
					if (dist <= renderDistance)
					{
						if (chunks.find({ camChunkX + x, 0, camChunkZ + z }) == chunks.end())
						{
							AddChunkToGenerate({ camChunkX + x, 0, camChunkZ + z });
						}
					}
				}
			}
		}

		{
			ZoneScopedN("Remove old chunks");
			for (auto it = chunks.begin(); it != chunks.end(); )
			{
				ChunkPos pos = it->first;
				Chunk* chunk = it->second;
				it++;

				float dist = sqrt(pow(abs(pos.x - camChunkX), 2) + pow(abs(pos.z - camChunkZ), 2));

				if (chunk->ready && dist > renderDistance)
				{
					// Out of range delete chunk
					delete chunk;
					chunks.erase(pos);
				}
			}
		}

		//{
		//	ZoneScopedN("Cleanup chunk data");
		//	for (auto it = chunkData.begin(); it != chunkData.end(); )
		//	{
		//		ChunkPos pos = it->first;
		//		ChunkData* data = it->second;
		//		// If you index at the end of the function, you risk accessed deleted data, by the erase function.
		//		// Do it now to guarantee valid iterator.
		//		++it;

		//		if (chunks.find(pos) == chunks.end()/* &&
		//			chunks.find({ pos.x + 1, pos.y, pos.z }) == chunks.end() &&
		//			chunks.find({ pos.x - 1, pos.y, pos.z }) == chunks.end() &&
		//			chunks.find({ pos.x, pos.y + 1, pos.z }) == chunks.end() &&
		//			chunks.find({ pos.x, pos.y - 1, pos.z }) == chunks.end() &&
		//			chunks.find({ pos.x, pos.y, pos.z + 1 }) == chunks.end() &&
		//			chunks.find({ pos.x, pos.y, pos.z - 1 }) == chunks.end()*/)
		//		{
		//			delete data;
		//			chunkData.erase(pos);
		//		}
		//	}
		//}
	}
}

void Planet::ChunkThreadGenerator(int threadId)
{
	tracy::SetThreadName(fmt::sprintf("Chunk Generator %i", threadId).c_str());
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	while (!shouldEnd)
	{
		Chunk* chunk;
		if (!generatorChunks.try_dequeue(chunk))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		ZoneScoped;

		// Set chunk data
		if (!chunk->chunkData->generated)
			WorldGen::GenerateChunkData(chunk->chunkPos, chunk->chunkData);

		//// Set top data
		//if (!chunk->upData->generated)
		//	WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x, chunk->chunkPos.y + 1, chunk->chunkPos.z }, chunk->upData);

		//// Set bottom data
		//if (!chunk->downData->generated)
		//	WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x, chunk->chunkPos.y - 1, chunk->chunkPos.z }, chunk->downData);

		//// Set north data
		//if (!chunk->northData->generated)
		//	WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x, chunk->chunkPos.y, chunk->chunkPos.z - 1 }, chunk->northData);

		//// Set south data
		//if (!chunk->southData->generated)
		//	WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x, chunk->chunkPos.y, chunk->chunkPos.z + 1 }, chunk->southData);

		//// Set east data
		//if (!chunk->eastData->generated)
		//	WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x + 1, chunk->chunkPos.y, chunk->chunkPos.z }, chunk->eastData);

		//// Set west data
		//if (!chunk->westData->generated)
		//	WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x - 1, chunk->chunkPos.y, chunk->chunkPos.z }, chunk->westData);

		// Generate chunk mesh
		chunk->GenerateChunkMesh();
	}
}

void Planet::AddChunkToGenerate(ChunkPos chunkPos)
{
	ZoneScoped;
	// Create chunk object
	Chunk* chunk = new Chunk(chunkPos, solidShader, waterShader);

	ChunkPos pos;

	chunk->chunkData = new ChunkData();

	//pos = ChunkPos{ chunkPos.x, 0, chunkPos.z };
	//if (chunkData.find(pos) == chunkData.end())
	//	chunkData[pos] = new ChunkData();
	//chunk->chunkData = chunkData[pos];

	//pos = ChunkPos{ chunkPos.x + 1, 0, chunkPos.z };
	//if (chunkData.find(pos) == chunkData.end())
	//	chunkData[pos] = new ChunkData();
	//chunk->eastData = chunkData[pos];

	//pos = ChunkPos{ chunkPos.x - 1, 0, chunkPos.z };
	//if (chunkData.find(pos) == chunkData.end())
	//	chunkData[pos] = new ChunkData();
	//chunk->westData = chunkData[pos];

	//pos = ChunkPos{ chunkPos.x, 0 + 1, chunkPos.z };
	//if (chunkData.find(pos) == chunkData.end())
	//	chunkData[pos] = new ChunkData();
	//chunk->upData = chunkData[pos];

	//pos = ChunkPos{ chunkPos.x, 0 - 1, chunkPos.z };
	//if (chunkData.find(pos) == chunkData.end())
	//	chunkData[pos] = new ChunkData();
	//chunk->downData = chunkData[pos];

	//pos = ChunkPos{ chunkPos.x, 0, chunkPos.z + 1 };
	//if (chunkData.find(pos) == chunkData.end())
	//	chunkData[pos] = new ChunkData();
	//chunk->southData = chunkData[pos];

	//pos = ChunkPos{ chunkPos.x, 0, chunkPos.z - 1 };
	//if (chunkData.find(pos) == chunkData.end())
	//	chunkData[pos] = new ChunkData();
	//chunk->northData = chunkData[pos];

	chunks[chunkPos] = chunk;
	generatorChunks.enqueue(chunk);
}

Chunk* Planet::GetChunk(ChunkPos chunkPos)
{
	//chunkMutex.lock();
	if (chunks.find(chunkPos) == chunks.end())
	{
		//chunkMutex.unlock();
		return nullptr;
	}
	else
	{
		//chunkMutex.unlock();
		return chunks.at(chunkPos);
	}
}

void Planet::ClearChunkQueue()
{
	lastCamX++;
}