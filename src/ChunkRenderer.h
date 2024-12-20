#pragma once

#include "Chunk.h"

namespace ChunkRenderer
{
	void RenderOpaque(
		std::unordered_map<ChunkPos, Chunk*, ChunkPosHash>& chunks,
		Shader* solidShader, Shader* billboardShader,
		uint32_t& out_chunksLoading, uint32_t& out_chunksRendered)
	{
		ZoneScoped;

		out_chunksLoading = 0;
		out_chunksRendered = 0;
		glDisable(GL_BLEND);

		{
			ZoneScopedN("Solid Rendering");
			solidShader->use();
			GLint modelLoc = solidShader->GetUniformLocation("model");
			for (auto& [chunkPos, chunk] : chunks)
			{
				if (!chunk->ready)
				{
					out_chunksLoading++;
					chunk->PrepareRender();
				}

				if (!chunk->numTrianglesMain)
					continue;

				solidShader->setMat4x4(modelLoc, chunk->modelMatrix);

				glBindVertexArray(chunk->mainVAO);
				glDrawElements(GL_TRIANGLES, chunk->numTrianglesMain, GL_UNSIGNED_INT, 0);

				out_chunksRendered++;
			}
		}

		{
			ZoneScopedN("Billboard Rendering");
			billboardShader->use();

			GLint modelLoc = billboardShader->GetUniformLocation("model");
			glDisable(GL_CULL_FACE);
			for (auto& [chunkPos, chunk] : chunks)
			{
				if (!chunk->numTrianglesBillboard)
					continue;

				billboardShader->setMat4x4(modelLoc, chunk->modelMatrix);

				glBindVertexArray(chunk->billboardVAO);
				glDrawElements(GL_TRIANGLES, chunk->numTrianglesBillboard, GL_UNSIGNED_INT, 0);
			}
			glEnable(GL_CULL_FACE);
		}
	}

	void RenderTransparent(
		std::unordered_map<ChunkPos, Chunk*, ChunkPosHash>& chunks,
		Shader* waterShader)
	{
		ZoneScoped;

		glEnable(GL_BLEND);
		waterShader->use();

		GLint modelLoc = waterShader->GetUniformLocation("model");
		for (auto& [chunkPos, chunk] : chunks)
		{
			if (!chunk->ready)
			{
				chunk->PrepareRender();
			}

			if (!chunk->numTrianglesWater)
				continue;

			waterShader->setMat4x4(modelLoc, chunk->modelMatrix);

			glBindVertexArray(chunk->waterVAO);
			glDrawElements(GL_TRIANGLES, chunk->numTrianglesWater, GL_UNSIGNED_INT, 0);

			//chunk->RenderWater(waterShader);
		}


		//glBindVertexArray(Planet::planet->transparentVAO);

		//GLint modelLoc = waterShader->GetUniformLocation("model");
		//glm::mat4x4 matrices[128];
		//GLsizei triangleCounts[128];
		//GLint triangleOffsets[128];
		//int drawCount = 0;
		//for (auto& [chunkPos, chunk] : chunks)
		//{
		//	if (!chunk->ready)
		//	{
		//		chunk->PrepareRender();
		//	}

		//	if (!chunk->numTrianglesWater)
		//		continue;


		//	drawCount++;

		//	if (drawCount == 128)
		//	{
		//		waterShader->setMat4x4s(modelLoc, drawCount, matrices);
		//		glMultiDrawElementsBaseVertex(GL_TRIANGLES, triangleCounts, GL_UNSIGNED_INT, 0, drawCount, triangleOffsets);
		//		drawCount = 0;
		//	}
		//}

		//if (drawCount != 0)
		//{
		//	waterShader->setMat4x4s(modelLoc, drawCount, matrices);
		//	glMultiDrawElementsBaseVertex(GL_TRIANGLES, triangleCounts, GL_UNSIGNED_INT, 0, drawCount, triangleOffsets);
		//}
	}
}