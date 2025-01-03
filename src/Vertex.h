#pragma once

#include <glm/glm.hpp>

struct Vertex
{
	glm::i8vec3 pos;
	glm::i8vec2 texGrid;
	char direction;

	Vertex(glm::i8vec3 _pos, glm::i8vec2 _texGrid, char _direction = 0)
		: pos(_pos), texGrid(_texGrid), direction(_direction)
	{ }
};

struct BillboardVertex
{
	glm::u16vec3 pos;
	glm::i8vec2 texGrid;
	char direction;

	BillboardVertex(glm::lowp_fvec3 _pos, glm::i8vec2 _texGrid, char _direction = 0)
		: texGrid(_texGrid), direction(_direction)
	{
		pos.x = glm::detail::toFloat16(_pos.x);
		pos.y = glm::detail::toFloat16(_pos.y);
		pos.z = glm::detail::toFloat16(_pos.z);
	}
};
