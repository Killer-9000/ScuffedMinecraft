#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#ifdef LINUX
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <utils/stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <chrono>
#include <thread>

#include "graphics/Buffer.h"
#include "graphics/Shader.h"
#include "graphics/Framebuffer.h"
#include "graphics/Misc.h"
#include "graphics/VertexArrayObject.h"
#include "Camera.h"
#include "Planet.h"
#include "Blocks.h"
#include "Physics.h"

#include <glad/glad.h>

#include <tracy/Tracy.hpp>

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

float deltaTime = 0.0f;
float lastFrame = 0.0f;
int fpsCount = 0;
std::chrono::steady_clock::time_point fpsStartTime;
float avgFps = 0;
float lowestFps = -1;
float highestFps = -1;
float lastX = NAN, lastY = NAN;

bool menuMode = false;
bool escapeDown = false;
bool f1Down = false;

// Window settings
GLsizei windowX = 1920;
GLsizei windowY = 1080;
bool vsync = true;

uint16_t selectedBlock = 1;

bool uiEnabled = true;

Camera camera;

GLuint FBO;
GLuint framebufferTexture;
GLuint depthTexture;

float rectangleVertices[] =
{
	 // Coords     // TexCoords
	 1.0f, -1.0f,  1.0f, 0.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	-1.0f,  1.0f,  0.0f, 1.0f,

	 1.0f,  1.0f,  1.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	-1.0f,  1.0f,  0.0f, 1.0f
};

//float outlineVertices[] = 
//{
//	-.001f, -.001f, -.001f,  1.001f, -.001f, -.001f,
//    1.001f, -.001f, -.001f,  1.001f, 1.001f, -.001f,
//	1.001f, 1.001f, -.001f,  -.001f, 1.001f, -.001f,
//	-.001f, 1.001f, -.001f,  -.001f, -.001f, -.001f,
//	   		   		   							
//	-.001f, -.001f, -.001f,  -.001f, -.001f, 1.001f,
//	-.001f, -.001f, 1.001f,  -.001f, 1.001f, 1.001f,
//	-.001f, 1.001f, 1.001f,  -.001f, 1.001f, -.001f,
//	   		   		   							
//	1.001f, -.001f, -.001f,  1.001f, -.001f, 1.001f,
//	1.001f, -.001f, 1.001f,  1.001f, 1.001f, 1.001f,
//	1.001f, 1.001f, 1.001f,  1.001f, 1.001f, -.001f,
//	   		   		   							
//	-.001f, -.001f, 1.001f,  1.001f, -.001f, 1.001f,
//	-.001f, 1.001f, 1.001f,  1.001f, 1.001f, 1.001f,
//};

float outlineVertices[] =
{
	 1.001f,  1.001f, -0.001f,
	 1.001f, -0.001f, -0.001f,
	 1.001f,  1.001f,  1.001f,
	 1.001f, -0.001f,  1.001f,
	-0.001f,  1.001f, -0.001f,
	-0.001f, -0.001f, -0.001f,
	-0.001f,  1.001f,  1.001f,
	-0.001f, -0.001f,  1.001f
};

uint16_t outlineIndicies[] =
{
	4, 2, 0,
	2, 7, 3,
	6, 5, 7,
	1, 7, 5,
	0, 3, 1,
	4, 1, 5,
	4, 6, 2,
	2, 6, 7,
	6, 4, 5,
	1, 3, 7,
	0, 2, 3,
	4, 0, 1
};

float crosshairVertices[] =
{
	//windowX / 2 - 13.5f, windowY / 2 - 13.5f,  0.0f, 0.0f,
	//windowX / 2 + 13.5f, windowY / 2 - 13.5f,  1.0f, 0.0f,
	//windowX / 2 + 13.5f, windowY / 2 + 13.5f,  1.0f, 1.0f,
	//			  					 
	//windowX / 2 - 13.5f, windowY / 2 - 13.5f,  0.0f, 0.0f,
	//windowX / 2 - 13.5f, windowY / 2 + 13.5f,  0.0f, 1.0f,
	//windowX / 2 + 13.5f, windowY / 2 + 13.5f,  1.0f, 1.0f,

	0.45f, 0.45f,  0.0f, 0.0f,
	0.55f, 0.45f,  1.0f, 0.0f,
	0.55f, 0.55f,  1.0f, 1.0f,

	0.45f, 0.45f,  0.0f, 0.0f,
	0.45f, 0.55f,  0.0f, 1.0f,
	0.55f, 0.55f,  1.0f, 1.0f,
};

void APIENTRY glDebugOutput(GLenum source,
	GLenum type,
	unsigned int id,
	GLenum severity,
	GLsizei length,
	const char* message,
	const void* userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}

int main(int argc, char *argv[])
{
	ZoneScoped;
#ifdef LINUX
	char* resolved_path = realpath(argv[0], NULL);
	if (resolved_path == NULL) {
		printf("%s: Please do not place binary in PATH\n", argv[0]);
		exit(1);
	}
	size_t resolved_length = strlen(resolved_path);
	// remove executable from path
	for (size_t i = resolved_length; i > 0; i--) {
		if (resolved_path[i] == '/' && resolved_path[i + 1] != 0) {
			resolved_path[i + 1] = 0;
			resolved_length = i;
			break;
		}
	}
	char* assets_path = (char*)malloc(resolved_length + strlen("assets") + 2);
	strcpy(assets_path, resolved_path);
	strcpy(assets_path + resolved_length + 1, "assets");
	struct stat path_stat;
	if (stat(assets_path, &path_stat) == -1 || !S_ISDIR(path_stat.st_mode)) {
		printf("%s: Asset directory not found\n", argv[0]);
		exit(1);
	}
	free(assets_path);

	chdir(resolved_path);
	free(resolved_path);
#endif

	// Load Texture
	stbi_set_flip_vertically_on_load(true);

	// Initialize GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_DEPTH_BITS, 24);
	//glfwWindowHint(GLFW_DOUBLEBUFFER, 1);

	glfwWindowHint(GLFW_CONTEXT_DEBUG, true);

	// Create window
	GLFWwindow* window = glfwCreateWindow(windowX, windowY, "Scuffed Minecraft", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window\n";
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(vsync ? 1 : 0);

	// Initialize GLAD
	if (!gladLoadGL())
	{
		std::cout << "Failed to initialize GLAD\n";
		return -1;
	}

	// Configure viewport and rendering
	glViewport(0, 0, windowX, windowY);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}

	glClearColor(0.6f, 0.8f, 1.0f, 1.0f);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	glEnable(GL_DEPTH_TEST);

	// Create shaders
	Shader shader("assets/shaders/main_vert.glsl", "assets/shaders/main_frag.glsl");
	{
		ShaderBinder _(shader);
		_.setFloat("texMultiplier", 0.0625f);
	}

	Shader waterShader("assets/shaders/water_vert.glsl", "assets/shaders/water_frag.glsl");
	{
		ShaderBinder _(waterShader);
		_.setFloat("texMultiplier", 0.0625f);
	}

	Shader billboardShader("assets/shaders/billboard_vert.glsl", "assets/shaders/billboard_frag.glsl");
	{
		ShaderBinder _(billboardShader);
		_.setFloat("texMultiplier", 0.0625f);
	}

	Shader framebufferShader("assets/shaders/framebuffer_vert.glsl", "assets/shaders/framebuffer_frag.glsl");
	Shader outlineShader("assets/shaders/block_outline_vert.glsl", "assets/shaders/block_outline_frag.glsl");
	Shader crosshairShader("assets/shaders/crosshair_vert.glsl", "assets/shaders/crosshair_frag.glsl");

	// Create post-processing framebuffer
	glCreateFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	glCreateTextures(GL_TEXTURE_2D, 1, &framebufferTexture);
	glBindTexture(GL_TEXTURE_2D, framebufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windowX, windowY, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glNamedFramebufferTexture(FBO, GL_COLOR_ATTACHMENT0, framebufferTexture, 0);

	glCreateTextures(GL_TEXTURE_2D, 1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, windowX, windowY, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glNamedFramebufferTexture(FBO, GL_DEPTH_ATTACHMENT, depthTexture, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	{
		auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer error: " << std::hex << fboStatus << '\n';
	}

	VertexArrayObject rectVAO;
	Buffer rectVBO(GL_ARRAY_BUFFER, sizeof(rectangleVertices), rectangleVertices, GL_STATIC_DRAW);
	rectVAO.BindVertexBuffer(0, rectVBO, 0, 4 * sizeof(float));
	rectVAO.BindVertexBuffer(1, rectVBO, 0, 4 * sizeof(float));
	rectVAO.SetAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0);
	rectVAO.SetAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));

	{
		ShaderBinder _(framebufferShader);
		_.setInt("screenTexture", 0);
		_.setInt("depthTexture", 1);
	}

	VertexArrayObject outlineVAO;
	Buffer outlineVBO(GL_ARRAY_BUFFER, sizeof(outlineVertices), outlineVertices, GL_STATIC_DRAW);
	Buffer outlineEBO(GL_ELEMENT_ARRAY_BUFFER, sizeof(outlineIndicies), outlineIndicies, GL_STATIC_DRAW);
	outlineVAO.BindVertexBuffer(0, outlineVBO, 0, 3 * sizeof(float));
	outlineVAO.SetAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0);
	{
		VAOBinder _(outlineVAO);
		outlineEBO.Bind();
	}

	VertexArrayObject crosshairVAO;
	Buffer crosshairVBO(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
	crosshairVAO.BindVertexBuffer(0, rectVBO, 0, 4 * sizeof(float));
	crosshairVAO.BindVertexBuffer(1, rectVBO, 0, 4 * sizeof(float));
	crosshairVAO.SetAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0);
	crosshairVAO.SetAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));

	// Create terrain texture
	Texture2D texture("assets/sprites/block_map.png");
	texture.SetParameterI(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	texture.SetParameterI(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//texture.GenerateMipmaps();

	// Create crosshair texture
	Texture2D crosshairTexture("assets/sprites/crosshair.png");
	crosshairTexture.SetParameterI(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	crosshairTexture.SetParameterI(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//crosshairTexture.GenerateMipmaps();

	// Create camera
	camera = Camera(glm::vec3(CHUNK_WIDTH / 2, 72.0f, CHUNK_WIDTH / 2));

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	Planet::planet = new Planet(&shader, &waterShader, &billboardShader);

	glm::mat4 ortho = glm::ortho(0.0f, (float)windowX, (float)windowY, 0.0f, 0.1f, 10000.0f);

	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(nullptr);

	fpsStartTime = std::chrono::steady_clock::now();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while (!glfwWindowShouldClose(window))
	{
		FrameMark;

		// Delta Time
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// FPS Calculations
		float fps = 1.0f / deltaTime;
		if (lowestFps == -1 || fps < lowestFps)
			lowestFps = fps;
		if (highestFps == -1 || fps > highestFps)
			highestFps = fps;
		fpsCount++;
		std::chrono::steady_clock::time_point currentTimePoint = std::chrono::steady_clock::now();
		if (std::chrono::duration_cast<std::chrono::seconds>(currentTimePoint - fpsStartTime).count() >= 1)
		{
			avgFps = fpsCount;
			lowestFps = -1;
			highestFps = -1;
			fpsCount = 0;
			fpsStartTime = currentTimePoint;
		}

		{
			ShaderBinder _(waterShader);
			_.setFloat("time", currentFrame);
		}
		{
			ShaderBinder _(outlineShader);
			_.setFloat("time", currentFrame);
		}

		// Input
		processInput(window);

		// Rendering
		glEnable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		texture.BindUnit(0);

		// New ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			ZoneScopedN("Application::main VP uniforms");
			// Projection matrix
			glm::mat4 view = camera.GetViewMatrix();

			glm::mat4 projection;
			projection = glm::perspective(glm::radians(camera.Zoom), (float)windowX / windowY, 0.1f, 10000.0f);

			{
				ShaderBinder _(shader);
				_.setMat4x4("view", view);
				_.setMat4x4("projection", projection);
			}
			{
				ShaderBinder _(waterShader);
				_.setMat4x4("view", view);
				_.setMat4x4("projection", projection);
			}
			{
				ShaderBinder _(billboardShader);
				_.setMat4x4("view", view);
				_.setMat4x4("projection", projection);
			}
			{
				ShaderBinder _(outlineShader);
				_.setMat4x4("view", view);
				_.setMat4x4("projection", projection);
			}
		}

		Planet::planet->Update(camera.Position);

		// -- Render block outline -- //
		if (uiEnabled)
		{
			ZoneScopedN("Application::main block outline");

			// Get block position
			auto result = Physics::Raycast(camera.Position, camera.Front, 5);
			if (result.hit)
			{
				ShaderBinder _(outlineShader);

				// Set outline view to position
				glm::mat4x4 model = glm::mat4x4(1);
				model = glm::translate(model, { result.blockX, result.blockY, result.blockZ });
				_.setBool("chunkBorder", false);
				_.setMat4x4("model", model);

				// Render
				glDisable(GL_CULL_FACE);
				outlineVAO.Bind();
				glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);

				glEnable(GL_CULL_FACE);
			}
		}

		static bool showChunkBorders = false;

		// -- Render chunk outline -- //
		if (showChunkBorders)
		{
			ZoneScopedN("Application::main chunk outline");

			ShaderBinder _(outlineShader);
			glDisable(GL_CULL_FACE);

			VAOBinder _1(outlineVAO);

			// Set outline view to position
			glm::mat4x4 model = glm::mat4x4(1);
			model = glm::scale(model, { CHUNK_WIDTH, CHUNK_HEIGHT, CHUNK_WIDTH });
			model = glm::translate(model, { Planet::planet->camChunkX, 0, Planet::planet->camChunkZ });
			_.setBool("chunkBorder", true);

			// Render
			_.setMat4x4("model", model);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);

			model = glm::translate(model, { 1, 0, 0 });
			_.setMat4x4("model", model);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);

			model = glm::translate(model, { -2, 0, 0 });
			_.setMat4x4("model", model);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);

			model = glm::translate(model, { 1, 0, 1 });
			_.setMat4x4("model", model);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);

			model = glm::translate(model, { 0, 0, -2 });
			_.setMat4x4("model", model);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);

			glEnable(GL_CULL_FACE);
		}

		{
			ZoneScopedN("Application::main post processing");

			ShaderBinder _(framebufferShader);

			// -- Post Processing Stuff -- //

			// Check if player is underwater
			int blockX = camera.Position.x < 0 ? camera.Position.x - 1 : camera.Position.x;
			int blockY = camera.Position.y < 0 ? camera.Position.y - 1 : camera.Position.y;
			int blockZ = camera.Position.z < 0 ? camera.Position.z - 1 : camera.Position.z;

			int chunkX = blockX < 0 ? (int)floorf((float)blockX / CHUNK_WIDTH) : (int)((float)blockX / CHUNK_WIDTH);
			int chunkY = blockY < 0 ? (int)floorf((float)blockY / CHUNK_HEIGHT) : (int)((float)blockY / CHUNK_HEIGHT);
			int chunkZ = blockZ < 0 ? (int)floorf((float)blockZ / CHUNK_WIDTH) : (int)((float)blockZ / CHUNK_WIDTH);

			int localBlockX = blockX - (chunkX * CHUNK_WIDTH);
			int localBlockY = blockY - (chunkY * CHUNK_HEIGHT);
			int localBlockZ = blockZ - (chunkZ * CHUNK_WIDTH);

			Chunk::Ptr chunk = Planet::planet->GetChunk(ChunkPos(chunkX, chunkY, chunkZ));
			if (chunk != nullptr)
			{
				unsigned int blockType = chunk->GetBlockAtPos(
					localBlockX,
					localBlockY,
					localBlockZ);

				if (Blocks::blocks[blockType].blockType == Block::LIQUID)
				{
					_.setBool("underwater", true);
				}
				else
				{
					_.setBool("underwater", false);
				}
			}
			else
				_.setBool("underwater", false);

			// Post Processing
			Framebuffer::ClearBind();

			glDisable(GL_DEPTH_TEST);
			VAOBinder _1(rectVAO);

			glBindTextureUnit(0, framebufferTexture);
			glBindTextureUnit(1, depthTexture);

			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		if (uiEnabled)
		{
			{
				ZoneScopedN("Application::main crosshair");

				// -- Render Crosshair -- //

				ShaderBinder _(crosshairShader);
				VAOBinder _1(crosshairVAO);

				// Render
				crosshairTexture.BindUnit(0);

				ScopedEnable _2(GL_CULL_FACE, false);
				ScopedEnable _3(GL_DEPTH_TEST, false);
				ScopedEnable _4(GL_BLEND);
				ScopedEnable _5(GL_COLOR_LOGIC_OP);

				_.setMat4x4("projection", ortho);
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}

			ZoneScopedN("Application::main UI");

			// Draw ImGui UI
			ImGui::Begin("Test");
			ImGui::Text("FPS: %d (Avg: %d, Min: %d, Max: %d)", (int)fps, (int)avgFps, (int)lowestFps, (int)highestFps);
			ImGui::Text("MS: %f", deltaTime * 100.0f);
			if (ImGui::Checkbox("VSYNC", &vsync))
				glfwSwapInterval(vsync ? 1 : 0);
			ImGui::Text("Chunks: %d (%d rendered, %d loading)", Planet::planet->numChunks, Planet::planet->numChunksRendered, Planet::planet->numChunks - Planet::planet->numChunksRendered);
			ImGui::Text("Position: x: %f, y: %f, z: %f", camera.Position.x, camera.Position.y, camera.Position.z);
			ImGui::Text("Direction: x: %f, y: %f, z: %f", camera.Front.x, camera.Front.y, camera.Front.z);
			ImGui::Text("Selected Block: %s", Blocks::blocks[selectedBlock].blockName.c_str());
			static int oldRenderDistance = Planet::planet->renderDistance;
			if (ImGui::SliderInt("Render Distance", &Planet::planet->renderDistance, 1, 128))
			{
				if (Planet::planet->renderDistance < oldRenderDistance)
					Planet::planet->ClearChunkQueue();
				Planet::planet->UpdateChunkQueue();
			}
			ImGui::Checkbox("Unload chunks", &Planet::planet->deleteChunks);
			ImGui::Checkbox("Load chunks", &Planet::planet->loadChunks);
			//if (ImGui::SliderInt("Render Height", &Planet::planet->renderHeight, 0, 10))
			//	Planet::planet->ClearChunkQueue();
			ImGui::Checkbox("Show chunk borders", &showChunkBorders);
			ImGui::Checkbox("Use absolute Y axis for camera vertical movement", &camera.absoluteVerticalMovement);
			ImGui::End();
		}

		{
			ZoneScopedN("Application::main Render UI");
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}

		{
			ZoneScopedN("Application::main Poll and swap");
			glfwSwapBuffers(window);
			glfwPollEvents();
		}

		//std::cout << camera.Position.x << ", " << camera.Position.y << ", " << camera.Position.z << '\n';
	}

	delete Planet::planet;

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	windowX = width;
	windowY = height;
	glViewport(0, 0, windowX, windowY);

	GLint lastTexture = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);

	// resize framebuffer texture
	glBindTexture(GL_TEXTURE_2D, framebufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windowX, windowY, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glNamedFramebufferTexture(FBO, GL_COLOR_ATTACHMENT0, framebufferTexture, 0);

	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, windowX, windowY, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
	glNamedFramebufferTexture(FBO, GL_DEPTH_ATTACHMENT, depthTexture, 0);

	glBindTexture(GL_TEXTURE_2D, lastTexture);
}

void processInput(GLFWwindow* window)
{
	// Pause
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		if (escapeDown)
			return;

		escapeDown = true;
		menuMode = !menuMode;
		glfwSetInputMode(window, GLFW_CURSOR, menuMode ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
		lastX = NAN;
		lastY = NAN;
		//glfwSetWindowShouldClose(window, true);
	}
	else
		escapeDown = false;

	// UI Toggle
	if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
	{
		if (f1Down)
			return;

		f1Down = true;
		uiEnabled = !uiEnabled;
	}
	else
		f1Down = false;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			camera.ProcessKeyboard(FORWARD_NO_Y, deltaTime);
		else
			camera.ProcessKeyboard(FORWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (menuMode)
		return;

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		auto result = Physics::Raycast(camera.Position, camera.Front, 5);
		if (!result.hit)
			return;

		result.chunk->UpdateBlock(result.localBlockX, result.localBlockY, result.localBlockZ, 0);
	}
	else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
	{
		auto result = Physics::Raycast(camera.Position, camera.Front, 5);
		if (!result.hit)
			return;

		selectedBlock = result.chunk->GetBlockAtPos(result.localBlockX, result.localBlockY, result.localBlockZ);
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		auto result = Physics::Raycast(camera.Position, camera.Front, 5);
		if (!result.hit)
			return;

		float distX = result.hitPos.x - (result.blockX + .5f);
		float distY = result.hitPos.y - (result.blockY + .5f);
		float distZ = result.hitPos.z - (result.blockZ + .5f);

		int blockX = result.blockX;
		int blockY = result.blockY;
		int blockZ = result.blockZ;
		
		// Choose face to place on
		if (abs(distX) > abs(distY) && abs(distX) > abs(distZ))
			blockX += (distX > 0 ? 1 : -1);
		else if (abs(distY) > abs(distX) && abs(distY) > abs(distZ))
			blockY += (distY > 0 ? 1 : -1);
		else
			blockZ += (distZ > 0 ? 1 : -1);

		int chunkX = blockX < 0 ? floorf(blockX / (float)CHUNK_WIDTH) : blockX / (int)CHUNK_WIDTH;
		int chunkY = blockY < 0 ? floorf(blockY / (float)CHUNK_HEIGHT) : blockY / (int)CHUNK_HEIGHT;
		int chunkZ = blockZ < 0 ? floorf(blockZ / (float)CHUNK_WIDTH) : blockZ / (int)CHUNK_WIDTH;

		int localBlockX = blockX - (chunkX * CHUNK_WIDTH);
		int localBlockY = blockY - (chunkY * CHUNK_HEIGHT);
		int localBlockZ = blockZ - (chunkZ * CHUNK_WIDTH);

		Chunk::Ptr chunk = Planet::planet->GetChunk(ChunkPos(chunkX, chunkY, chunkZ));
		if (chunk)
		{
			uint16_t blockToReplace = chunk->GetBlockAtPos(localBlockX, localBlockY, localBlockZ);
			if (blockToReplace == 0 || Blocks::blocks[blockToReplace].blockType == Block::LIQUID)
				chunk->UpdateBlock(localBlockX, localBlockY, localBlockZ, selectedBlock);
		}
	}
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (menuMode)
		return;

	if (isnan(lastX) || isnan(lastY))
	{
		lastX = (float)xpos;
		lastY = (float)ypos;
	}

	float xoffset = (float)xpos - lastX;
	float yoffset = lastY - (float)ypos;
	lastX = (float)xpos;
	lastY = (float)ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}