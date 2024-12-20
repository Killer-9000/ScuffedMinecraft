#pragma once

struct Vertex
{
	char posX, posY, posZ;
	char texGridX, texGridY;
	char direction;

	Vertex(char _posX, char _posY, char _posZ, char _texGridX, char _texGridY, char _direction = 0)
	{
		posX = _posX;
		posY = _posY;
		posZ = _posZ;

		texGridX = _texGridX;
		texGridY = _texGridY;

		direction = _direction;
	}
};

struct BillboardVertex
{
	float posX, posY, posZ;
	char texGridX, texGridY;
	char direction;

	BillboardVertex(float _posX, float _posY, float _posZ, char _texGridX, char _texGridY, char _direction = 0)
	{
		posX = _posX;
		posY = _posY;
		posZ = _posZ;

		texGridX = _texGridX;
		texGridY = _texGridY;

		direction = _direction;
	}
};
