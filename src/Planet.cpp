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
	uint32_t threads = 20;// std::thread::hardware_concurrency();
	//threads /= 2;
	for (int i = 0; i < threads; i++)
		generatorThreads.emplace_back(&Planet::ChunkThreadGenerator, this, i);
#endif

	{
		DrawingData& data = opaqueDrawingData;
		data.vbo.Resize(16 * 1024 * 1024 * sizeof(Vertex), GL_DYNAMIC_DRAW);
		data.ebo.Resize(16 * 1024 * 1024 * sizeof(uint32_t), GL_DYNAMIC_DRAW);

		data.vao.BindVertexBuffer(0, data.vbo, 0, sizeof(Vertex));
		data.vao.BindVertexBuffer(1, data.vbo, 0, sizeof(Vertex));
		data.vao.BindVertexBuffer(2, data.vbo, 0, sizeof(Vertex));
		data.vao.SetAttribPointerI(0, 3, GL_BYTE, offsetof(Vertex, pos));
		data.vao.SetAttribPointerI(1, 2, GL_BYTE, offsetof(Vertex, texGrid));
		data.vao.SetAttribPointerI(2, 1, GL_BYTE, offsetof(Vertex, direction));
	}

	{
		DrawingData& data = billboardDrawingData;
		data.vbo.Resize(16 * 1024 * 1024 * sizeof(BillboardVertex), GL_DYNAMIC_DRAW);
		data.ebo.Resize(16 * 1024 * 1024 * sizeof(uint32_t), GL_DYNAMIC_DRAW);

		data.vao.BindVertexBuffer(0, data.vbo, 0, sizeof(BillboardVertex));
		data.vao.BindVertexBuffer(1, data.vbo, 0, sizeof(BillboardVertex));
		data.vao.BindVertexBuffer(2, data.vbo, 0, sizeof(BillboardVertex));
		data.vao.SetAttribPointer(0, 3, GL_HALF_FLOAT, GL_FALSE, offsetof(BillboardVertex, pos));
		data.vao.SetAttribPointerI(1, 2, GL_BYTE, offsetof(BillboardVertex, texGrid));
		data.vao.SetAttribPointerI(2, 1, GL_BYTE, offsetof(BillboardVertex, direction));
	}

	{
		DrawingData& data = transparentDrawingData;
		data.vbo.Resize(16 * 1024 * 1024 * sizeof(Vertex), GL_DYNAMIC_DRAW);
		data.ebo.Resize(16 * 1024 * 1024 * sizeof(uint32_t), GL_DYNAMIC_DRAW);

		data.vao.BindVertexBuffer(0, data.vbo, 0, sizeof(Vertex));
		data.vao.BindVertexBuffer(1, data.vbo, 0, sizeof(Vertex));
		data.vao.BindVertexBuffer(2, data.vbo, 0, sizeof(Vertex));
		data.vao.SetAttribPointerI(0, 3, GL_BYTE, offsetof(Vertex, pos));
		data.vao.SetAttribPointerI(1, 2, GL_BYTE, offsetof(Vertex, texGrid));
		data.vao.SetAttribPointerI(2, 1, GL_BYTE, offsetof(Vertex, direction));
	}

	chunkComputeShader.ComputeShader("assets/shaders/chunk_compute.glsl");
	chunkComputeShader.Compile();
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

	{
		//ScopedPolygonMode _(GL_LINE);
		//glLineWidth(3);

		numChunks = chunks.size();
		ChunkRenderer::RenderOpaque(chunks, solidShader, billboardShader, chunksLoading, numChunksRendered);
		ChunkRenderer::RenderTransparent(chunks, waterShader);
	}

	// Check if camera moved to new chunk
	if (camChunkX != lastCamX || camChunkY != lastCamY || camChunkZ != lastCamZ)
	{
		ZoneScopedN("Moved");
		// Player moved chunks, start new chunk queue
		lastCamX = camChunkX;
		lastCamY = camChunkY;
		lastCamZ = camChunkZ;

		if (loadChunks)
		{
			ZoneScopedN("Add new chunks");
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
				Chunk::Ptr chunk = it->second;
				it++;

				float dist = sqrt(pow(abs(pos.x - camChunkX), 2) + pow(abs(pos.z - camChunkZ), 2));

				// Out of range delete chunk
				if (chunk->ready && dist > renderDistance && chunk->ready)
				{
					chunk->markedForDelete = true;
					chunks.erase(pos);
				}
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
		Chunk::Ptr chunk;
		if (generatorChunks.try_dequeue(chunk))
		{
			ZoneScoped;

			std::array<Chunk::Ptr, 4> surroundingChunks;

			{
				ZoneScopedN("Checks");
				if (!chunk || chunk->markedForDelete)
				{
					continue;
				}
				if (!chunk->generatorMutex.try_lock())
				{
					continue;
				}

				auto getChunk = [this](Chunk::Ptr base, int x, int y, int z)->Chunk::Ptr
					{
						auto itr = chunks.find({ x + base->chunkPos.x, 0, z + base->chunkPos.z });
						if (itr == chunks.end())
							return nullptr;
						if (!itr->second->chunkData.generated)
							return nullptr;
						if (itr->second->markedForDelete)
							return nullptr;
						return itr->second;
					};

				surroundingChunks =
				{
					getChunk(chunk, -1, 0,  0), // left
					getChunk(chunk, 1, 0,  0),  // right
					getChunk(chunk, 0, 0, 1),   // front
					getChunk(chunk, 0, 0, -1),  // back
				};

				if ((chunk->surroundedChunks[0] == (bool)surroundingChunks[0]
					&& chunk->surroundedChunks[1] == (bool)surroundingChunks[1]
					&& chunk->surroundedChunks[2] == (bool)surroundingChunks[2]
					&& chunk->surroundedChunks[3] == (bool)surroundingChunks[3])
					&& chunk->generated)
				{
					chunk->generatorMutex.unlock();
					continue;
				}
			}

			// Really just the initial generation of chunk
			// 
			// Generate blocks using noisemaps
			if (!chunk->chunkData.generated)
			{
				chunk->chunkData.blockIDs = new uint16_t[CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH];
				WorldGen::GenerateChunkData(chunk->chunkPos, &chunk->chunkData);
				chunk->edgeUpdate = true;
			}

			// Convert blocks to triangle mesh

			bool firstTime = !chunk->generated;

			chunk->surroundedChunks[0] = surroundingChunks[0] != nullptr;
			chunk->surroundedChunks[1] = surroundingChunks[1] != nullptr;
			chunk->surroundedChunks[2] = surroundingChunks[2] != nullptr;
			chunk->surroundedChunks[3] = surroundingChunks[3] != nullptr;
			//chunkMeshMutex.lock();
			chunk->GenerateChunkMesh(surroundingChunks[0], surroundingChunks[1], surroundingChunks[2], surroundingChunks[3]);
			//chunkMeshMutex.unlock();

			if (chunk->edgeUpdate)
			{
				if (surroundingChunks[0])
					AddChunkToGenerate(surroundingChunks[0]);
				if (surroundingChunks[1])
					AddChunkToGenerate(surroundingChunks[1]);
				if (surroundingChunks[2])
					AddChunkToGenerate(surroundingChunks[2]);
				if (surroundingChunks[3])
					AddChunkToGenerate(surroundingChunks[3]);
			}
			else
			{
				if (surroundingChunks[0] && !surroundingChunks[0]->surroundedChunks[1])
					AddChunkToGenerate(surroundingChunks[0]);
				if (surroundingChunks[1] && !surroundingChunks[1]->surroundedChunks[0])
					AddChunkToGenerate(surroundingChunks[1]);
				if (surroundingChunks[2] && !surroundingChunks[2]->surroundedChunks[3])
					AddChunkToGenerate(surroundingChunks[2]);
				if (surroundingChunks[3] && !surroundingChunks[3]->surroundedChunks[2])
					AddChunkToGenerate(surroundingChunks[3]);
			}

			chunk->edgeUpdate = false;
			chunk->generatorMutex.unlock();
		}

#if !SYNCRONOUS_GENERATION
		Sleep(1);
	}
#endif
}

void Planet::AddChunkToGenerate(Chunk::Ptr chunk)
{
	chunk->generated = false;
	generatorChunks.enqueue(chunk);
}

void Planet::AddChunkToGenerate(ChunkPos chunkPos)
{
	ZoneScoped;

	auto itr = chunks.find(chunkPos);
	if (itr == chunks.end())
	{
		Chunk::Ptr chunk = std::make_shared<Chunk>(chunkPos, solidShader, waterShader);
		chunks[chunkPos] = chunk;
		AddChunkToGenerate(chunk);
	}
	else
	{
		AddChunkToGenerate(itr->second);
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

Chunk::Ptr Planet::GetChunk(ChunkPos chunkPos)
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
