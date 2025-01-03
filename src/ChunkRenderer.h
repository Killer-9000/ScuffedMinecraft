#pragma once

#include "Chunk.h"
#include "graphics/Misc.h"

namespace ChunkRenderer
{
	void RenderOpaque(
		std::unordered_map<ChunkPos, Chunk::Ptr, ChunkPosHash>& chunks,
		Shader* solidShader, Shader* billboardShader,
		uint32_t& out_chunksLoading, uint32_t& out_chunksRendered)
	{
		ZoneScoped;

		out_chunksLoading = 0;
		out_chunksRendered = 0;

		ScopedEnable _(GL_BLEND, false);

		for (auto& [chunkPos, chunk] : chunks)
		{
			if (!chunk->ready)
			{
				out_chunksLoading++;
				chunk->PrepareRender();
			}
		}

		{
			ScopedEnable _1(GL_CULL_FACE);

			ZoneScopedN("Solid Rendering");

			ShaderBinder _2(solidShader);

			Planet::DrawingData& data = Planet::planet->opaqueDrawingData;

			VAOBinder _3(data.vao);
			BufferBinder _4(data.ebo);
			BufferBinder _5(data.ibo);

			GLint modelLoc = solidShader->GetUniformLocation("models");
			constexpr uint32_t MAX_DRAW_COMMANDS = 512;
			DrawElementsIndirectCommand commands[MAX_DRAW_COMMANDS];
			glm::vec3 matrices[MAX_DRAW_COMMANDS];
			int drawCount = 0;
			for (auto& [chunkPos, chunk] : chunks)
			{
				if (!chunk->ready || !chunk->opaqueEle)
					continue;

				matrices[drawCount] = chunk->worldPos;
				commands[drawCount].count = chunk->opaqueEle->size / sizeof(uint32_t);
				commands[drawCount].instanceCount = 1;
				commands[drawCount].firstIndex = chunk->opaqueEle->offset / sizeof(uint32_t);
				commands[drawCount].baseVertex = chunk->opaqueTri->offset / sizeof(Vertex);
				commands[drawCount].baseInstance = drawCount;

				drawCount++;
				out_chunksRendered++;

				if (drawCount == MAX_DRAW_COMMANDS)
				{
					_2.setFloat3s(modelLoc, drawCount, matrices);
					data.ibo.SetData(sizeof(commands), commands, GL_DYNAMIC_DRAW);
					glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, drawCount, sizeof(DrawElementsIndirectCommand));
					drawCount = 0;
				}
			}

			if (drawCount != 0)
			{
				_2.setFloat3s(modelLoc, drawCount, matrices);
				data.ibo.SetData(sizeof(commands), commands, GL_DYNAMIC_DRAW);
				glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, drawCount, sizeof(DrawElementsIndirectCommand));
			}
		}

		{
			ScopedEnable _1(GL_CULL_FACE, false);

			ZoneScopedN("Billboard Rendering");

			ShaderBinder _2(billboardShader);

			Planet::DrawingData& data = Planet::planet->billboardDrawingData;

			VAOBinder _3(data.vao);
			BufferBinder _4(data.ebo);
			BufferBinder _5(data.ibo);

			GLint modelLoc = billboardShader->GetUniformLocation("models");
			constexpr uint32_t MAX_DRAW_COMMANDS = 512;
			DrawElementsIndirectCommand commands[MAX_DRAW_COMMANDS];
			glm::vec3 matrices[MAX_DRAW_COMMANDS];
			int drawCount = 0;
			for (auto& [chunkPos, chunk] : chunks)
			{
				if (!chunk->ready || !chunk->billboardEle)
					continue;

				matrices[drawCount] = chunk->worldPos;
				commands[drawCount].count = chunk->billboardEle->size / sizeof(uint32_t);
				commands[drawCount].instanceCount = 1;
				commands[drawCount].firstIndex = chunk->billboardEle->offset / sizeof(uint32_t);
				commands[drawCount].baseVertex = chunk->billboardTri->offset / sizeof(BillboardVertex);
				commands[drawCount].baseInstance = drawCount;

				drawCount++;

				if (drawCount == MAX_DRAW_COMMANDS)
				{
					_2.setFloat3s(modelLoc, drawCount, matrices);
					data.ibo.SetData(sizeof(commands), commands, GL_DYNAMIC_DRAW);
					glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, drawCount, sizeof(DrawElementsIndirectCommand));
					drawCount = 0;
				}
			}

			if (drawCount != 0)
			{
				_2.setFloat3s(modelLoc, drawCount, matrices);
				data.ibo.SetData(sizeof(commands), commands, GL_DYNAMIC_DRAW);
				glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, drawCount, sizeof(DrawElementsIndirectCommand));
			}
		}
	}

	void RenderTransparent(
		std::unordered_map<ChunkPos, Chunk::Ptr, ChunkPosHash>& chunks,
		Shader* waterShader)
	{
		ZoneScoped;

		ShaderBinder _(waterShader);

		ScopedEnable _1(GL_BLEND);
		ScopedEnable _2(GL_CULL_FACE, false);

		Planet::DrawingData& data = Planet::planet->transparentDrawingData;

		VAOBinder _3(data.vao);
		BufferBinder _4(data.ebo);
		BufferBinder _5(data.ibo);

		GLint modelLoc = waterShader->GetUniformLocation("models");
		constexpr uint32_t MAX_DRAW_COMMANDS = 512;
		DrawElementsIndirectCommand commands[MAX_DRAW_COMMANDS];
		glm::vec3 matrices[MAX_DRAW_COMMANDS];
		int drawCount = 0;
		for (auto& [chunkPos, chunk] : chunks)
		{
			if (!chunk->ready || !chunk->waterEle)
				continue;

			matrices[drawCount] = chunk->worldPos;
			commands[drawCount].count = chunk->waterEle->size / sizeof(uint32_t);
			commands[drawCount].instanceCount = 1;
			commands[drawCount].firstIndex = chunk->waterEle->offset / sizeof(uint32_t);
			commands[drawCount].baseVertex = chunk->waterTri->offset / sizeof(Vertex);
			commands[drawCount].baseInstance = drawCount;

			drawCount++;

			if (drawCount == MAX_DRAW_COMMANDS)
			{
				_.setFloat3s(modelLoc, drawCount, matrices);
				data.ibo.SetData(sizeof(commands), commands, GL_DYNAMIC_DRAW);
				glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, drawCount, sizeof(DrawElementsIndirectCommand));
				drawCount = 0;
			}
		}

		if (drawCount != 0)
		{
			_.setFloat3s(modelLoc, drawCount, matrices);
			data.ibo.SetData(sizeof(commands), commands, GL_DYNAMIC_DRAW);
			glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, drawCount, sizeof(DrawElementsIndirectCommand));
		}
	}
}