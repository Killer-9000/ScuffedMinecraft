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
			//chunkQueue.push({ camChunkX, camChunkY, camChunkZ });

			// Starting from the center
			int k = 0;
			for (int i = 0; i < renderDistance && k < 64; i++)
			{
				for (float angle = 0.0f; angle < 2 * 3.141592; angle += 0.01f)
				{
					int x = i * cosf(angle) + camChunkX;
					int z = i * sinf(angle) + camChunkZ;

					if (chunks.find({ x, 0, z }) == chunks.end())
					{
						AddChunkToGenerate({ x, 0, z });
						if (k++ >= 64)
							break;
					}

					//for (int y = -renderHeight + camChunkY; y <= renderHeight + camChunkY; y++)
					//{
					//	if (chunks.find({ x, y, z }) == chunks.end())
					//	{
					//		AddChunkToGenerate({ x, y, z });
					//		if (k++ >= 128)
					//			break;
					//	}
					//}
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

				if (chunk->ready && (abs(pos.x - camChunkX) > renderDistance + 3 ||
					abs(pos.z - camChunkZ) > renderDistance + 3))
				{
					// Out of range
					// Add chunk data to delete queue
					chunkDataDeleteQueue.push({ pos.x,     pos.y,     pos.z });
					chunkDataDeleteQueue.push({ pos.x + 1, pos.y,     pos.z });
					chunkDataDeleteQueue.push({ pos.x - 1, pos.y,     pos.z });
					chunkDataDeleteQueue.push({ pos.x,     pos.y + 1, pos.z });
					chunkDataDeleteQueue.push({ pos.x,     pos.y - 1, pos.z });
					chunkDataDeleteQueue.push({ pos.x,     pos.y,     pos.z + 1 });
					chunkDataDeleteQueue.push({ pos.x,     pos.y,     pos.z - 1 });

					// Delete chunk
					delete chunk;
					chunks.erase(pos);
				}
			}
		}

		{
			ZoneScopedN("Cleanup chunk data");
			for (auto it = chunkData.begin(); it != chunkData.end(); )
			{
				ChunkPos pos = it->first;
				// If you index at the end of the function, you risk accessed deleted data, by the erase function.
				// Do it now to guarantee valid iterator.
				++it;

				if (chunks.find(pos) == chunks.end() &&
					chunks.find({ pos.x + 1, pos.y, pos.z }) == chunks.end() &&
					chunks.find({ pos.x - 1, pos.y, pos.z }) == chunks.end() &&
					chunks.find({ pos.x, pos.y + 1, pos.z }) == chunks.end() &&
					chunks.find({ pos.x, pos.y - 1, pos.z }) == chunks.end() &&
					chunks.find({ pos.x, pos.y, pos.z + 1 }) == chunks.end() &&
					chunks.find({ pos.x, pos.y, pos.z - 1 }) == chunks.end())
				{
					delete chunkData.at(pos);
					chunkData.erase(pos);
				}
			}
		}

		//chunkMutex.unlock();

		//// Current chunk
		//chunkMutex.lock();
		//chunkQueue = {};
		//if (chunks.find({ camChunkX, camChunkY, camChunkZ }) == chunks.end())
		//	chunkQueue.push({ camChunkX, camChunkY, camChunkZ });
		//
		//for (int r = 0; r < renderDistance; r++)
		//{
		//	// Add middle chunks
		//	for (int y = 0; y <= renderHeight; y++)
		//	{
		//		chunkQueue.push({ camChunkX,     camChunkY + y, camChunkZ + r });
		//		chunkQueue.push({ camChunkX + r, camChunkY + y, camChunkZ });
		//		chunkQueue.push({ camChunkX,     camChunkY + y, camChunkZ - r });
		//		chunkQueue.push({ camChunkX - r, camChunkY + y, camChunkZ });
		//
		//		if (y > 0)
		//		{
		//			chunkQueue.push({ camChunkX,     camChunkY - y, camChunkZ + r });
		//			chunkQueue.push({ camChunkX + r, camChunkY - y, camChunkZ });
		//			chunkQueue.push({ camChunkX,     camChunkY - y, camChunkZ - r });
		//			chunkQueue.push({ camChunkX - r, camChunkY - y, camChunkZ });
		//		}
		//	}
		//
		//	// Add edges
		//	for (int e = 1; e < r; e++)
		//	{
		//		for (int y = 0; y <= renderHeight; y++)
		//		{
		//			chunkQueue.push({ camChunkX + e, camChunkY + y, camChunkZ + r });
		//			chunkQueue.push({ camChunkX - e, camChunkY + y, camChunkZ + r });
		//
		//			chunkQueue.push({ camChunkX + r, camChunkY + y, camChunkZ + e });
		//			chunkQueue.push({ camChunkX + r, camChunkY + y, camChunkZ - e });
		//
		//			chunkQueue.push({ camChunkX + e, camChunkY + y, camChunkZ - r });
		//			chunkQueue.push({ camChunkX - e, camChunkY + y, camChunkZ - r });
		//
		//			chunkQueue.push({ camChunkX - r, camChunkY + y, camChunkZ + e });
		//			chunkQueue.push({ camChunkX - r, camChunkY + y, camChunkZ - e });
		//
		//			if (y > 0)
		//			{
		//				chunkQueue.push({ camChunkX + e, camChunkY - y, camChunkZ + r });
		//				chunkQueue.push({ camChunkX - e, camChunkY - y, camChunkZ + r });
		//
		//				chunkQueue.push({ camChunkX + r, camChunkY - y, camChunkZ + e });
		//				chunkQueue.push({ camChunkX + r, camChunkY - y, camChunkZ - e });
		//
		//				chunkQueue.push({ camChunkX + e, camChunkY - y, camChunkZ - r });
		//				chunkQueue.push({ camChunkX - e, camChunkY - y, camChunkZ - r });
		//
		//				chunkQueue.push({ camChunkX - r, camChunkY - y, camChunkZ + e });
		//				chunkQueue.push({ camChunkX - r, camChunkY - y, camChunkZ - e });
		//			}
		//		}
		//	}
		//
		//	// Add corners
		//	for (int y = 0; y <= renderHeight; y++)
		//	{
		//		chunkQueue.push({ camChunkX + r, camChunkY + y, camChunkZ + r });
		//		chunkQueue.push({ camChunkX + r, camChunkY + y, camChunkZ - r });
		//		chunkQueue.push({ camChunkX - r, camChunkY + y, camChunkZ + r });
		//		chunkQueue.push({ camChunkX - r, camChunkY + y, camChunkZ - r });
		//
		//		if (y > 0)
		//		{
		//			chunkQueue.push({ camChunkX + r, camChunkY - y, camChunkZ + r });
		//			chunkQueue.push({ camChunkX + r, camChunkY - y, camChunkZ - r });
		//			chunkQueue.push({ camChunkX - r, camChunkY - y, camChunkZ + r });
		//			chunkQueue.push({ camChunkX - r, camChunkY - y, camChunkZ - r });
		//		}
		//	}
		//}
		//
		//chunkMutex.unlock();
	}

	//chunkMutex.lock();

	//int i = 0;
	//while (!chunkQueue.empty() && i++ < 256)
	//{
	//	// Check if chunk exists
	//	ChunkPos chunkPos = chunkQueue.front();
	//	chunkQueue.pop();
	//	if (chunks.find(chunkPos) != chunks.end())
	//		continue;

	//}

	//chunkMutex.unlock();
}

void Planet::ChunkThreadGenerator(int threadId)
{
	tracy::SetThreadName(fmt::sprintf("Chunk Generator %i", threadId).c_str());
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	while (!shouldEnd)
	{
		if (generatorChunks.size() == 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		if (!generatorMutex.try_lock())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		if (generatorChunks.size() == 0)
		{
			generatorMutex.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		ZoneScoped;

		Chunk* chunk = generatorChunks.front();
		generatorChunks.pop();
		generatorMutex.unlock();

		// Set chunk data
		if (!chunk->chunkData->generated)
			WorldGen::GenerateChunkData(chunk->chunkPos, chunk->chunkData);

		// Set top data
		if (!chunk->upData->generated)
			WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x, chunk->chunkPos.y + 1, chunk->chunkPos.z }, chunk->upData);

		// Set bottom data
		if (!chunk->downData->generated)
			WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x, chunk->chunkPos.y - 1, chunk->chunkPos.z }, chunk->downData);

		// Set north data
		if (!chunk->northData->generated)
			WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x, chunk->chunkPos.y, chunk->chunkPos.z - 1 }, chunk->northData);

		// Set south data
		if (!chunk->southData->generated)
			WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x, chunk->chunkPos.y, chunk->chunkPos.z + 1 }, chunk->southData);

		// Set east data
		if (!chunk->eastData->generated)
			WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x + 1, chunk->chunkPos.y, chunk->chunkPos.z }, chunk->eastData);

		// Set west data
		if (!chunk->westData->generated)
			WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x - 1, chunk->chunkPos.y, chunk->chunkPos.z }, chunk->westData);

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

	pos = ChunkPos{ chunkPos.x, 0, chunkPos.z };
	if (chunkData.find(pos) == chunkData.end())
		chunkData[pos] = new ChunkData();
	chunk->chunkData = chunkData[pos];

	pos = ChunkPos{ chunkPos.x + 1, 0, chunkPos.z };
	if (chunkData.find(pos) == chunkData.end())
		chunkData[pos] = new ChunkData();
	chunk->eastData = chunkData[pos];

	pos = ChunkPos{ chunkPos.x - 1, 0, chunkPos.z };
	if (chunkData.find(pos) == chunkData.end())
		chunkData[pos] = new ChunkData();
	chunk->westData = chunkData[pos];

	pos = ChunkPos{ chunkPos.x, 0 + 1, chunkPos.z };
	if (chunkData.find(pos) == chunkData.end())
		chunkData[pos] = new ChunkData();
	chunk->upData = chunkData[pos];

	pos = ChunkPos{ chunkPos.x, 0 - 1, chunkPos.z };
	if (chunkData.find(pos) == chunkData.end())
		chunkData[pos] = new ChunkData();
	chunk->downData = chunkData[pos];

	pos = ChunkPos{ chunkPos.x, 0, chunkPos.z + 1 };
	if (chunkData.find(pos) == chunkData.end())
		chunkData[pos] = new ChunkData();
	chunk->southData = chunkData[pos];

	pos = ChunkPos{ chunkPos.x, 0, chunkPos.z - 1 };
	if (chunkData.find(pos) == chunkData.end())
		chunkData[pos] = new ChunkData();
	chunk->northData = chunkData[pos];

	generatorMutex.lock();

	chunks[chunkPos] = chunk;
	generatorChunks.push(chunk);

	generatorMutex.unlock();
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