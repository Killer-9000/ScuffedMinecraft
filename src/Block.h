#pragma once

#include <string>

struct Block
{
public:
	enum BLOCK_TYPE
	{
		SOLID = 1,
		TRANSPARENT = 2,
		LEAVES = 4,
		BILLBOARD = 8,
		LIQUID = 16
	};

	char topMinX, topMinY, topMaxX, topMaxY;
	char bottomMinX, bottomMinY, bottomMaxX, bottomMaxY;
	char sideMinX, sideMinY, sideMaxX, sideMaxY;
	BLOCK_TYPE blockType;
	std::string blockName;

	Block(char minX, char minY, char maxX, char maxY, BLOCK_TYPE blockType, std::string blockName);
	Block(char topMinX, char topMinY, char topMaxX, char topMaxY, 
		char bottomMinX, char bottomMinY, char bottomMaxX, char bottomMaxY,
		char sideMinX, char sideMinY, char sideMaxX, char sideMaxY, BLOCK_TYPE blockType, std::string blockName);

};