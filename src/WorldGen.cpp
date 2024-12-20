#include "WorldGen.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <OpenSimplexNoise.h>
#include <tracy/Tracy.hpp>

#include "Blocks.h"
#include "Planet.h"

void WorldGen::GenerateChunkData(ChunkPos chunkPos, ChunkData* chunkData)
{
	ZoneScoped;

	// Init noise
	static thread_local OSN::Noise<2> noise2D(0L/*time(nullptr)*/);
	static thread_local OSN::Noise<3> noise3D(0L/*time(nullptr)*/);

	// Init noise settings
	static NoiseSettings surfaceSettings[]{
		{ 0.01f, 20.0f, 64 },
		{ 0.05f,  3.0f, 64 }
	};
	static int surfaceSettingsLength = sizeof(surfaceSettings) / sizeof(*surfaceSettings);

	static NoiseSettings caveSettings[]{
		{ 0.05f, 1.0f, 0, .5f, 0, 100 }
	};
	static int caveSettingsLength = sizeof(caveSettings) / sizeof(*caveSettings);

	static NoiseSettings oreSettings[]{
		{ 0.075f, 1.0f, 8.54f, .75f, 1, 0 }
	};
	static int oreSettingsLength = sizeof(oreSettings) / sizeof(*oreSettings);

	static SurfaceFeature surfaceFeatures[]{
		// Pond
		{
			{ 0.43f, 1.0f, 2.35f, .85f, 1, 0 },	// Noise
			{									// Blocks
				0, 0, 0,  0,  0,  0, 0,
				0, 0, 0,  0,  0,  0, 0,
				0, 0, 0,  13, 13, 0, 0,
				0, 0, 13, 13, 13, 0, 0,
				0, 0, 0,  13, 0,  0, 0,
				0, 0, 0,  0,  0,  0, 0,
				0, 0, 0,  0,  0,  0, 0,
				
				0, 2,  13, 13, 2,  0,  0,
				0, 2,  13, 13, 13, 2,  0,
				2, 13, 13, 13, 13, 13, 2,
				2, 13, 13, 13, 13, 13, 2,
				2, 13, 13, 13, 13, 13, 2,
				0, 2,  13, 13, 13, 2,  0,
				0, 0,  2,  13, 2,  0,  0,

				0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0,
			},								
			{									// Replace?
				false, false, false, false, false, false, false,
				false, false, false, false, false, false, false,
				false, false, false, true,  true,  false, false,
				false, false, true,  true,  true,  false, false,
				false, false, false, true,  false, false, false,
				false, false, false, false, false, false, false,
				false, false, false, false, false, false, false,
				
				false, false, true,  true, false, false, false,
				false, false, true,  true, true,  false, false,
				false, true,  true,  true, true,  true,  false,
				false, true,  true,  true, true,  true,  false,
				false, true,  true,  true, true,  true,  false,
				false, false, true,  true, true,  false, false,
				false, false, false, true, false, false, false,

				false, false, true,  true, false, false, false,
				false, false, true,  true, true,  true,  false,
				false, true,  true,  true, true,  true,  false,
				false, true,  true,  true, true,  true,  false,
				false, true,  true,  true, true,  true,  false,
				false, false, true,  true, true,  false, false,
				false, false, false, true, false, false, false,
			},							
			7, 3, 7,							// Size
			-3, -2, -3							// Offset
		},
		// Tree
		{
			{ 4.23f, 1.0f, 8.54f, .8f, 1, 0 },
			{ 
				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 1, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0,

				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 4, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0,

				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 4, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0,

				0, 5, 5, 5, 0,
				5, 5, 5, 5, 5,
				5, 5, 4, 5, 5,
				5, 5, 5, 5, 5,
				0, 5, 5, 5, 0,

				0, 5, 5, 5, 0,
				5, 5, 5, 5, 5,
				5, 5, 4, 5, 5,
				5, 5, 5, 5, 5,
				0, 5, 5, 5, 0,

				0, 0, 0, 0, 0,
				0, 0, 5, 0, 0,
				0, 5, 5, 5, 0,
				0, 0, 5, 0, 0,
				0, 0, 0, 0, 0,

				0, 0, 0, 0, 0,
				0, 0, 5, 0, 0,
				0, 5, 5, 5, 0,
				0, 0, 5, 0, 0,
				0, 0, 0, 0, 0,

			},
			{ 
				false, false, false, false, false,
				false, false, false, false, false,
				false, false, true,  false, false,
				false, false, false, false, false,
				false, false, false, false, false,

				false, false, false, false, false,
				false, false, false, false, false,
				false, false, true,  false, false,
				false, false, false, false, false,
				false, false, false, false, false,

				false, false, false, false, false,
				false, false, false, false, false,
				false, false, true,  false, false,
				false, false, false, false, false,
				false, false, false, false, false,

				false, false, false, false, false,
				false, false, false, false, false,
				false, false, true,  false, false,
				false, false, false, false, false,
				false, false, false, false, false,

				false, false, false, false, false,
				false, false, false, false, false,
				false, false, true,  false, false,
				false, false, false, false, false,
				false, false, false, false, false,

				false, false, false, false, false,
				false, false, false, false, false,
				false, false, false, false, false,
				false, false, false, false, false,
				false, false, false, false, false,

				false, false, false, false, false,
				false, false, false, false, false,
				false, false, false, false, false,
				false, false, false, false, false,
				false, false, false, false, false,
			},
			5,
			7,
			5,
			-2,
			0,
			-2
		},
		// Tall Grass
		{
			{ 1.23f, 1.0f, 4.34f, .6f, 1, 0 },
			{
				2, 7, 8
			},
			{
				false, false, false
			},
			1,
			3,
			1,
			0,
			0,
			0
		},
		// Grass
		{
			{ 2.65f, 1.0f, 8.54f, .5f, 1, 0 },
			{
				2, 6
			},
			{
				false, false
			},
			1,
			2,
			1,
			0,
			0,
			0
		},
		// Poppy
		{
			{ 5.32f, 1.0f, 3.67f, .8f, 1, 0 },
			{
				2, 9
			},
			{
				false, false
			},
			1,
			2,
			1,
			0,
			0,
			0
		},
		// White Tulip
		{
			{ 5.57f, 1.0f, 7.654f, .8f, 1, 0 },
			{
				2, 10
			},
			{
				false, false
			},
			1,
			2,
			1,
			0,
			0,
			0
		},
		// Pink Tulip
		{
			{ 4.94f, 1.0f, 2.23f, .8f, 1, 0 },
			{
				2, 11
			},
			{
				false, false
			},
			1,
			2,
			1,
			0,
			0,
			0
		},
		// Orange Tulip
		{
			{ 6.32f, 1.0f, 8.2f, .85f, 1, 0 },
			{
				2, 12
			},
			{
				false, false
			},
			1,
			2,
			1,
			0,
			0,
			0
		},
	};
	static int surfaceFeaturesLength = sizeof(surfaceFeatures) / sizeof(*surfaceFeatures);

	static int waterLevel = 64;

	// Account for chunk position
	int startX = chunkPos.x * CHUNK_WIDTH;
	int startY = chunkPos.y * CHUNK_HEIGHT;
	int startZ = chunkPos.z * CHUNK_WIDTH;

	//int currentIndex = 0;
	for (int x = 0; x < CHUNK_WIDTH; x++)
	{
		for (int z = 0; z < CHUNK_WIDTH; z++)
		{
			// Surface noise
			int noiseY = waterLevel;
			for (int i = 0; i < surfaceSettingsLength; i++)
			{
				noiseY += noise2D.eval(
					(float)((x + startX) * surfaceSettings[i].frequency) + surfaceSettings[i].offset,
					(float)((z + startZ) * surfaceSettings[i].frequency) + surfaceSettings[i].offset)
					* surfaceSettings[i].amplitude;
			}

			for (int y = 0; y < CHUNK_HEIGHT; y++)
			{
				// Cave noise
				bool cave = false;
				for (int i = 0; i < caveSettingsLength; i++)
				{
					if (y + startY > caveSettings[i].maxHeight)
						continue;

					float noiseCaves = noise3D.eval(
						(float)((x + startX) * caveSettings[i].frequency) + caveSettings[i].offset,
						(float)((y + startY) * caveSettings[i].frequency) + caveSettings[i].offset,
						(float)((z + startZ) * caveSettings[i].frequency) + caveSettings[i].offset)
						* caveSettings[i].amplitude;

					if (noiseCaves > caveSettings[i].chance)
					{
						cave = true;
						break;
					}
				}

				// Step 1: Terrain Shape (surface and caves) and Ores

				int localIndex = (y * CHUNK_WIDTH * CHUNK_WIDTH) + (x) + (z * CHUNK_WIDTH);

				// Sky and Caves
				if (y + startY > noiseY)
				{
					if (y + startY <= waterLevel)
						chunkData->blockIDs[localIndex] = Blocks::WATER;
					else
						chunkData->blockIDs[localIndex] = Blocks::AIR;
				}
				else if (cave)
					chunkData->blockIDs[localIndex] = Blocks::AIR;
				// Ground
				else
				{
					bool blockSet = false;
					for (int i = 0; i < oreSettingsLength; i++)
					{
						if (y + startY > oreSettings[i].maxHeight)
							continue;

						float noiseOre = noise3D.eval(
							(float)((x + startX) * oreSettings[i].frequency) + oreSettings[i].offset,
							(float)((y + startY) * oreSettings[i].frequency) + oreSettings[i].offset,
							(float)((z + startZ) * oreSettings[i].frequency) + oreSettings[i].offset)
							* oreSettings[i].amplitude;

						if (noiseOre > oreSettings[i].chance)
						{
							chunkData->blockIDs[localIndex] = oreSettings[i].block;
							blockSet = true;
							break;
						}
					}

					if (!blockSet)
					{
						if (y + startY == noiseY)
							if (noiseY > waterLevel + 1)
								chunkData->blockIDs[localIndex] = Blocks::GRASS_BLOCK;
							else
								chunkData->blockIDs[localIndex] = Blocks::SAND;
						else if (y + startY > waterLevel - 10)
							if (noiseY > waterLevel + 1)
								chunkData->blockIDs[localIndex] = Blocks::DIRT_BLOCK;
							else
								chunkData->blockIDs[localIndex] = Blocks::SAND;
						else
							chunkData->blockIDs[localIndex] = Blocks::STONE_BLOCK;
					}
				}

				//currentIndex++;
			}
		}
	}

	// Step 3: Surface Features
	for (int i = 0; i < surfaceFeaturesLength; i++)
	{
		for (int x = 0/*-surfaceFeatures[i].sizeX - surfaceFeatures[i].offsetX*/; x < CHUNK_WIDTH - surfaceFeatures[i].offsetX; x++)
		{
			for (int z = 0/*-surfaceFeatures[i].sizeZ - surfaceFeatures[i].offsetZ*/; z < CHUNK_WIDTH - surfaceFeatures[i].offsetZ; z++)
			{
				int noiseY = waterLevel;
				for (int s = 0; s < surfaceSettingsLength; s++)
				{
					noiseY += noise2D.eval(
						(float)((x + startX) * surfaceSettings[s].frequency) + surfaceSettings[s].offset,
						(float)((z + startZ) * surfaceSettings[s].frequency) + surfaceSettings[s].offset)
						* surfaceSettings[s].amplitude;
				}

				if (noiseY + surfaceFeatures[i].offsetY > startY + CHUNK_HEIGHT || noiseY + surfaceFeatures[i].sizeY + surfaceFeatures[i].offsetY < startY)
					continue;

				// Check if it's in water or on sand
				if (noiseY < waterLevel + 2)
					continue;
				
				// Check if it's in a cave
				bool cave = false;
				for (int i = 0; i < caveSettingsLength; i++)
				{
					if (noiseY + startY > caveSettings[i].maxHeight)
						continue;

					float noiseCaves = noise3D.eval(
						(float)((x + startX) * caveSettings[i].frequency) + caveSettings[i].offset,
						(float)((noiseY) * caveSettings[i].frequency) + caveSettings[i].offset,
						(float)((z + startZ) * caveSettings[i].frequency) + caveSettings[i].offset)
						* caveSettings[i].amplitude;

					if (noiseCaves > caveSettings[i].chance)
					{
						cave = true;
						break;
					}
				}

				if (cave)
					continue;

				float noise = noise2D.eval(
					(float)((x + startX) * surfaceFeatures[i].noiseSettings.frequency) + surfaceFeatures[i].noiseSettings.offset,
					(float)((z + startZ) * surfaceFeatures[i].noiseSettings.frequency) + surfaceFeatures[i].noiseSettings.offset);

				if (noise > surfaceFeatures[i].noiseSettings.chance)
				{
					int featureX = x + startX;
					int featureY = noiseY;
					int featureZ = z + startZ;

					for (int fX = 0; fX < surfaceFeatures[i].sizeX; fX++)
					{
						for (int fY = 0; fY < surfaceFeatures[i].sizeY; fY++)
						{
							for (int fZ = 0; fZ < surfaceFeatures[i].sizeZ; fZ++)
				 			{
								int localX = featureX + fX + surfaceFeatures[i].offsetX - startX;
								//std::cout << "FeatureX: " << featureX << ", fX: " << fX << ", startX: " << startX << ", localX: " << localX << '\n';
								int localY = featureY + fY + surfaceFeatures[i].offsetY - startY;
								//std::cout << "FeatureY: " << featureY << ", fY: " << fY << ", startY: " << startY << ", localY: " << localY << '\n';
								int localZ = featureZ + fZ + surfaceFeatures[i].offsetZ - startZ;
								//std::cout << "FeatureZ: " << featureZ << ", fZ: " << fZ << ", startZ: " << startZ << ", localZ: " << localZ << '\n';

								if (localX >= CHUNK_WIDTH || localX < 0)
									continue;
								if (localY >= CHUNK_HEIGHT || localY < 0)
									continue;
								if (localZ >= CHUNK_WIDTH || localZ < 0)
									continue;
								
								int featureIndex = fY * surfaceFeatures[i].sizeX * surfaceFeatures[i].sizeZ + 
									fX * surfaceFeatures[i].sizeZ  + 
									fZ;
								//std::cout << "Feature Index: " << featureIndex << '\n';
								int localIndex = (localY * CHUNK_WIDTH * CHUNK_WIDTH) + (localX) + (localZ * CHUNK_WIDTH);
								//int localIndex = localX * CHUNK_WIDTH * CHUNK_WIDTH + localZ * CHUNK_HEIGHT + localY;
								//std::cout << "Local Index: " << localIndex << ", Max Index: " << chunkData->blockIDs->size() << '\n';

								if (surfaceFeatures[i].replaceBlock[featureIndex] || chunkData->blockIDs[localIndex] == 0)
									chunkData->blockIDs[localIndex] = surfaceFeatures[i].blocks[featureIndex];
							}
						}
					}
				}
			}
		}
	}

	//std::vector<uint16_t> pallet;
	//for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE; i++)
	//{
	//	if (std::find(pallet.begin(), pallet.end(), chunkData->blockIDs[i]) == pallet.end())
	//		pallet.emplace_back(chunkData->blockIDs[i]);
	//}

	//chunkData->pallet = new uint16_t[pallet.size()];
	//for (int i = 0; i < pallet.size(); i++)
	//	chunkData->pallet[i] = pallet[i];

	//int palletBitcount = 1, i;
	//if (pallet.size() > 1)
	//{
	//	for (i = 0; i < 32; i++)
	//	{
	//		if ((1 << i) & pallet.size())
	//			palletBitcount = i + 1;
	//	}
	//}

	//chunkData->blockIdxs = Bitset(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * palletBitcount);

	////int amountOfBytes = ceilf((float)(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE) / 8 * pallet.size());
	////amountOfBytes = (amountOfBytes + (amountOfBytes % 2)) + 1; // pad it to uint16
	////chunkData->blockIdxs = new uint8_t[amountOfBytes];
	//for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE; i++)
	//{
	//	int idx = -1;
	//	for (int j = 0; j < pallet.size(); j++)
	//	{
	//		if (chunkData->blockIDs[i] == pallet[j])
	//		{
	//			idx = j;
	//			break;
	//		}
	//	}

	//	assert(idx != -1 && "Failed to find block in pallet??");

	//	chunkData->blockIdxs.Set(idx + 1, (size_t)(i * palletBitcount), palletBitcount);
	//}

	chunkData->generated = true;
}