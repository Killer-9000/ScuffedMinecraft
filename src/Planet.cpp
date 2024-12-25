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

#define SYNCRONOUS_GENERATION 0

Planet* Planet::planet = nullptr;

//static const unsigned int CHUNK_SIZE = 32;

// Public
Planet::Planet(Shader* solidShader, Shader* waterShader, Shader* billboardShader)
	: solidShader(solidShader), waterShader(waterShader), billboardShader(billboardShader)
{
#if !SYNCRONOUS_GENERATION
	uint32_t threads = 8;// std::thread::hardware_concurrency();
	//threads /= 2;
	for (int i = 0; i < threads; i++)
		generatorThreads.emplace_back(&Planet::ChunkThreadGenerator, this, i);
#endif

	{
		DrawingData& data = opaqueDrawingData;
		data.vbo.Resize(16 * 1024 * 1024 * sizeof(Vertex), GL_DYNAMIC_DRAW);
		data.ebo.Resize(16 * 1024 * 1024 * sizeof(uint32_t), GL_DYNAMIC_DRAW);
		data.ibo.Resize(128 * sizeof(DrawElementsIndirectCommand), GL_DYNAMIC_DRAW);

		data.vao.BindVertexBuffer(0, data.vbo, 0, sizeof(Vertex));
		data.vao.BindVertexBuffer(1, data.vbo, 0, sizeof(Vertex));
		data.vao.BindVertexBuffer(2, data.vbo, 0, sizeof(Vertex));
		data.vao.SetAttribPointer(0, 3, GL_BYTE, GL_FALSE, offsetof(Vertex, posX));
		data.vao.SetAttribPointer(1, 2, GL_BYTE, GL_FALSE, offsetof(Vertex, texGridX));
		data.vao.SetAttribPointerI(2, 1, GL_BYTE, offsetof(Vertex, direction));
	}

	{
		DrawingData& data = billboardDrawingData;
		data.vbo.Resize(16 * 1024 * 1024 * sizeof(BillboardVertex), GL_DYNAMIC_DRAW);
		data.ebo.Resize(16 * 1024 * 1024 * sizeof(uint32_t), GL_DYNAMIC_DRAW);
		data.ibo.Resize(128 * sizeof(DrawElementsIndirectCommand), GL_DYNAMIC_DRAW);

		data.vao.BindVertexBuffer(0, data.vbo, 0, sizeof(BillboardVertex));
		data.vao.BindVertexBuffer(1, data.vbo, 0, sizeof(BillboardVertex));
		data.vao.BindVertexBuffer(2, data.vbo, 0, sizeof(BillboardVertex));
		data.vao.SetAttribPointer(0, 3, GL_FLOAT, GL_FALSE, offsetof(BillboardVertex, posX));
		data.vao.SetAttribPointer(1, 2, GL_BYTE, GL_FALSE, offsetof(BillboardVertex, texGridX));
		data.vao.SetAttribPointerI(2, 1, GL_BYTE, offsetof(BillboardVertex, direction));
	}

	{
		DrawingData& data = transparentDrawingData;
		data.vbo.Resize(16 * 1024 * 1024 * sizeof(Vertex), GL_DYNAMIC_DRAW);
		data.ebo.Resize(16 * 1024 * 1024 * sizeof(uint32_t), GL_DYNAMIC_DRAW);
		data.ibo.Resize(128 * sizeof(DrawElementsIndirectCommand), GL_DYNAMIC_DRAW);

		data.vao.BindVertexBuffer(0, data.vbo, 0, sizeof(Vertex));
		data.vao.BindVertexBuffer(1, data.vbo, 0, sizeof(Vertex));
		data.vao.BindVertexBuffer(2, data.vbo, 0, sizeof(Vertex));
		data.vao.SetAttribPointer(0, 3, GL_BYTE, GL_FALSE, offsetof(Vertex, posX));
		data.vao.SetAttribPointer(1, 2, GL_BYTE, GL_FALSE, offsetof(Vertex, texGridX));
		data.vao.SetAttribPointerI(2, 1, GL_BYTE, offsetof(Vertex, direction));
	}
}

Planet::~Planet()
{
	shouldEnd = true;
#if !SYNCRONOUS_GENERATION
	for (auto& thread : generatorThreads)
		thread.join();
#endif
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
		if (loadChunks)
		{
			ZoneScopedN("Add new chunks");
			//if (chunks.find({ camChunkX, 0, camChunkZ }) == chunks.end())
			//	AddChunkToGenerate({ camChunkX, 0, camChunkZ });

			// 33333
			// 32223
			// 32123
			// 32223
			// 33333

			// Starting from the center
			for (int i = 0; i < renderDistance; i++)
			{
				for (int x = -i; x < i + 1; x++)
					for (int z = -i; z < i + 1; z++)
					{
						if ((x == i || x == -i) || (z == i || z == -i))
						{
							float dist = sqrt(pow(abs(x), 2) + pow(abs(z), 2));
							if (dist <= renderDistance)
							{
								if (chunks.find({ camChunkX + x, 0, camChunkZ + z }) == chunks.end())
									AddChunkToGenerate({ camChunkX + x, 0, camChunkZ + z });
								if (chunks.find({ camChunkX - x, 0, camChunkZ - z }) == chunks.end())
									AddChunkToGenerate({ camChunkX - x, 0, camChunkZ - z });
							}
						}
					}
			}
		}

		if (deleteChunks)
		{
			ZoneScopedN("Remove old chunks");
			for (auto it = chunks.begin(); it != chunks.end(); )
			{
				ChunkPos pos = it->first;
				Chunk* chunk = it->second;
				it++;

				float dist = sqrt(pow(abs(pos.x - camChunkX), 2) + pow(abs(pos.z - camChunkZ), 2));

				// Out of range delete chunk
				if (chunk->ready && dist > renderDistance)
				{
					chunk->markedForDelete = true;
				}
			}
			if (chunkDeletionMutex.try_lock())
			{
				for (auto it = chunks.begin(); it != chunks.end(); )
				{
					ChunkPos pos = it->first;
					Chunk* chunk = it->second;
					it++;
					if (chunk->markedForDelete)
					{
						delete chunk;
						chunks.erase(pos);
					}
				}
				chunkDeletionMutex.unlock();
			}
		}
	}


#if SYNCRONOUS_GENERATION
	ChunkThreadGenerator(-1);
#endif
}

void Planet::ChunkThreadGenerator(int threadId)
{
	using namespace std::chrono_literals;

#if !SYNCRONOUS_GENERATION
	tracy::SetThreadName(fmt::sprintf("Chunk Generator %i", threadId).c_str());
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif

#if !SYNCRONOUS_GENERATION
	while (!shouldEnd)
	{
#endif
		chunkDeletionMutex.lock();

		Chunk* chunk;
		if (generatorChunks.try_dequeue(chunk))
		{
			if (!chunk || chunk->markedForDelete)
				continue;

			chunkDeletionMutex.unlock();

			// Really just the initial generation of chunk
			// 
			// Generate blocks using noisemaps
			if (!chunk->chunkData.generated)
			{
				chunkBlockMutex.lock();
				chunk->chunkData.blockIDs = new uint16_t[CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH];
				WorldGen::GenerateChunkData(chunk->chunkPos, &chunk->chunkData);
				chunk->edgeUpdate = true;
				chunkBlockMutex.unlock();
			}

			//if (!chunk->generatingMutex.try_lock())
			//{
			//	chunk->generated = false;
			//	generatorChunks.enqueue(chunk);
			//	continue;
			//}


			// Convert blocks to triangle mesh
			if (!chunk->generated)
			{
				auto getChunk = [this](Chunk* base, int x, int y, int z)->Chunk*
					{
						auto itr = chunks.find({x + base->chunkPos.x, 0, z + base->chunkPos.z});
						if (itr == chunks.end())
							return nullptr;
						if (!itr->second->chunkData.generated)
							return nullptr;
						if (itr->second->markedForDelete)
							return nullptr;
						return itr->second;
					};

				std::array<Chunk*, 12> surroundingChunks =
				{
					//   A
					//  629
					// 40 17
					//  538
					//   B

					getChunk(chunk, -1, 0,  0), // left
					getChunk(chunk, 1, 0,  0),  // right
					getChunk(chunk, 0, 0, 1),   // front
					getChunk(chunk, 0, 0, -1),  // back

					getChunk(chunk, -2, 0,  0), // left left
					getChunk(chunk, -1, 0, -1), // back left
					getChunk(chunk, -1, 0,  1), // front left

					getChunk(chunk, 2, 0,  0), // right right
					getChunk(chunk, 1, 0, -1), // back right
					getChunk(chunk, 1, 0,  1), // front right

					getChunk(chunk, 0, 0, 2), // front front

					getChunk(chunk, 0, 0, -2), // back back
				};

				chunkMeshMutex.lock();
				chunkBlockMutex.lock_shared();
				chunk->GenerateChunkMesh(surroundingChunks[0], surroundingChunks[1], surroundingChunks[2], surroundingChunks[3]);

				//if (surroundingChunks[0] && chunk->edgeUpdate)
				//	surroundingChunks[0]->GenerateChunkMesh(surroundingChunks[4], chunk, surroundingChunks[6], surroundingChunks[5]);

				//if (surroundingChunks[1] && chunk->edgeUpdate)
				//	surroundingChunks[1]->GenerateChunkMesh(chunk, surroundingChunks[7], surroundingChunks[9], surroundingChunks[8]);

				//if (surroundingChunks[2] && chunk->edgeUpdate)
				//	surroundingChunks[2]->GenerateChunkMesh(surroundingChunks[6], surroundingChunks[9], surroundingChunks[10], chunk);

				//if (surroundingChunks[3] && chunk->edgeUpdate)
				//	surroundingChunks[3]->GenerateChunkMesh(surroundingChunks[5], surroundingChunks[8], chunk, surroundingChunks[11]);

				chunkBlockMutex.unlock_shared();
				chunkMeshMutex.unlock();

				//chunk->chunkData.Compress();

				chunk->edgeUpdate = false;
			}
			// Currently generating, so requeue.
			else
			{
				generatorChunks.enqueue(chunk);
			}

			//chunk->generatingMutex.unlock();

		}
		else
			chunkDeletionMutex.unlock();
		//// Set top data
		//if (!chunk->upData->generated)
		//	WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x, chunk->chunkPos.y + 1, chunk->chunkPos.z }, chunk->upData);
		//
		//// Set bottom data
		//if (!chunk->downData->generated)
		//	WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x, chunk->chunkPos.y - 1, chunk->chunkPos.z }, chunk->downData);
		//
		//// Set north data
		//if (!chunk->northData->generated)
		//	WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x, chunk->chunkPos.y, chunk->chunkPos.z - 1 }, chunk->northData);
		//
		//// Set south data
		//if (!chunk->southData->generated)
		//	WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x, chunk->chunkPos.y, chunk->chunkPos.z + 1 }, chunk->southData);
		//
		//// Set east data
		//if (!chunk->eastData->generated)
		//	WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x + 1, chunk->chunkPos.y, chunk->chunkPos.z }, chunk->eastData);
		//
		//// Set west data
		//if (!chunk->westData->generated)
		//	WorldGen::GenerateChunkData(ChunkPos{ chunk->chunkPos.x - 1, chunk->chunkPos.y, chunk->chunkPos.z }, chunk->westData);

#if !SYNCRONOUS_GENERATION
		std::this_thread::sleep_for(1ms);
	}
#endif
}

void Planet::AddChunkToGenerate(ChunkPos chunkPos)
{
	ZoneScoped;

	auto itr = chunks.find(chunkPos);
	if (itr == chunks.end())
	{
		Chunk* chunk = new Chunk(chunkPos, solidShader, waterShader);
		chunks[chunkPos] = chunk;
		generatorChunks.enqueue(chunk);
	}
	else
	{
		itr->second->generated = false;
		generatorChunks.enqueue(itr->second);
	}

	// Create chunk object

	//if (chunks.find({ chunkPos.x - 1, chunkPos.y, chunkPos.z }) != chunks.end())
	//{
	//	chunk->surroundingChunks[1] = chunks[{chunkPos.x - 1, chunkPos.y, chunkPos.z}];
	//	chunk->surroundingChunks[1]->surroundingChunks[0] = chunk;
	//}
	//if (chunks.find({ chunkPos.x + 1, chunkPos.y, chunkPos.z }) != chunks.end())
	//{
	//	chunk->surroundingChunks[0] = chunks[{chunkPos.x + 1, chunkPos.y, chunkPos.z}];
	//	chunk->surroundingChunks[0]->surroundingChunks[1] = chunk;
	//}
	//if (chunks.find({ chunkPos.x, chunkPos.y, chunkPos.z - 1 }) != chunks.end())
	//{
	//	chunk->surroundingChunks[2] = chunks[{chunkPos.x, chunkPos.y, chunkPos.z - 1 }];
	//	chunk->surroundingChunks[2]->surroundingChunks[3] = chunk;
	//}
	//if (chunks.find({ chunkPos.x, chunkPos.y, chunkPos.z + 1 }) != chunks.end())
	//{
	//	chunk->surroundingChunks[3] = chunks[{chunkPos.x, chunkPos.y, chunkPos.z + 1 }];
	//	chunk->surroundingChunks[3]->surroundingChunks[2] = chunk;
	//}
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
