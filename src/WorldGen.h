#pragma once

#include <vector>
#include "NoiseSettings.h"
#include "SurfaceFeature.h"
#include "ChunkPos.h"
#include "glm/glm.hpp"

struct ChunkData;

namespace WorldGen
{
	void GenerateChunkData(ChunkPos chunkPos, ChunkData* chunkData);
}