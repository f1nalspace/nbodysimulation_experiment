/*
-------------------------------------------------------------------------------------------------------------------
Multi-Threaded N-Body 2D Smoothed Particle Hydrodynamics Fluid Simulation based on paper "Particle-based Viscoelastic Fluid Simulation" by Simon Clavet, Philippe Beaudoin, and Pierre Poulin.

Version 1.2

A experiment about creating a two-way particle simulation in 4 different programming styles to see the difference in performance and maintainability.
The core math is same for all implementations, including rendering and threading.

Demos:

1. Object oriented style 1 (Naive)
2. Object oriented style 2 (Public, reserved vectors, fixed grid, no unneccesary classes or pointers)
3. Object oriented style 3 (Structs only, no virtual function calls, reserved vectors, fixed grid)
4. Data oriented style with 8/16 byte aligned structures

How to compile:

Compile main.cpp only and link with opengl, freeglut and glew.

Benchmark:

There is a benchmark recording and rendering built-in.

To start a benchmark hit "B" key.
To stop a benchmark hit "Escape" key.

Notes:

- Collision detection is discrete, therefore particles may pass through bodies when they are too thin and particles too fast.

Todo:

- Fix crash when using glDrawElements in combination of glTexcoordPointer (VerticesDraw command type)

- Replace GLUT with FPL
- Migrate all GUI/Text rendering to imGUI
- External particle forces
- Add value labels on benchmark chart
- Migrate to modern opengl 3.3+

Version History:

1.2:
- Using command buffer instead of immediate rendering, so we render only in main.cpp
- Rendering text using stb_truetype

1.1:
- Improved benchmark functionality and rendering

1.0:
- Added integrated benchmark functionality

0.9:
- Initial version

License:

MIT License
Copyright (c) 2017 Torsten Spaete
-------------------------------------------------------------------------------------------------------------------
*/
#include <GL/glew.h>
#if _WIN32
// @NOTE(final): windef.h defines min/max macros defined in lowerspace, this will break std::min/max so we have to tell the header we dont want that macros!
#define NOMINMAX
#include <GL/wglew.h>
#endif
#include <GL/freeglut.h>
#include <chrono>
#include <stack>

#include "app.cpp"
#include "utils.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <imgui/stb_truetype.h>

static Application *globalApp = nullptr;
static float lastFrameTime = 0.0f;
static uint64_t lastFrameCycles = 0;
static uint64_t lastCycles = 0;
static std::chrono::time_point<std::chrono::steady_clock> lastFrameClock;

static void ResizeFromGLUT(int width, int height) {
	globalApp->Resize(width, height);
}

#define ENABLE_TEXCOORDARRAY 0
#define ENABLE_DRAWELEMENTS 0
#define ENABLE_COLORARRAY 1
#define ENABLE_TEXTURES 1

static void OpenGLPopVertexIndexArray(std::stack<Render::VertexIndexArrayHeader *> &stack) {
	if (stack.size() > 0) {
		Render::VertexIndexArrayHeader *header = stack.top();
		stack.pop();

#if ENABLE_COLORARRAY
		if (header->colors != nullptr) {
			glDisableClientState(GL_COLOR_ARRAY);
		}
#endif

#if ENABLE_TEXCOORDARRAY
		if (header->texcoords != nullptr) {
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
#endif
		glDisableClientState(GL_VERTEX_ARRAY);
	}
}

static void OpenGLPushVertexIndexArray(std::stack<Render::VertexIndexArrayHeader *> &stack, Render::VertexIndexArrayHeader *header) {
	GLsizei vertexStride = (GLsizei)header->vertexStride;
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, vertexStride, header->vertices);

#if ENABLE_TEXCOORDARRAY
	if (header->texcoords != nullptr) {
		assert(header->texcoords != header->vertices);
		GLsizei texcoordStride = (GLsizei)header->texcoordStride;
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(2, GL_FLOAT, texcoordStride, header->texcoords);
	}
#endif

#if ENABLE_COLORARRAY
	if (header->colors != nullptr) {
		assert(header->colors != header->vertices);
		GLsizei colorStride = (GLsizei)header->colorStride;
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, colorStride, header->colors);
	}
#endif

	stack.push(header);
}

static void OpenGLAllocateTexture(Render::CommandBuffer *commandBuffer, Render::TextureOperationAllocate &allocate) {
	assert(sizeof(GLuint) <= sizeof(Render::TextureHandle *));
	assert((allocate.width > 0) && (allocate.height > 0));
	size_t textureSizeRGBA = allocate.width * allocate.height * 4;
	uint8_t *texturePixelsRGBA = PushSize<uint8_t>(&commandBuffer->textureData, textureSizeRGBA);
	// @TODO: Handle bottom-up (!isTopDown) by setting the initial source pixel and swap sign in source-increment!
	switch (allocate.bytesPerPixel) {
		case 1:
		{
			// Alpha > RGBA
			size_t sourceIncrement = 1;
			size_t sourceScanline = allocate.width;
			uint32_t *destPixel32 = (uint32_t *)texturePixelsRGBA;
			for (size_t y = 0; y < allocate.height; ++y) {
				uint8_t *sourceRow8 = (uint8_t *)allocate.data + (sourceScanline * (!allocate.isTopDown ? allocate.height - 1 - y : y));
				uint8_t *sourcePixel8 = sourceRow8;
				for (size_t x = 0; x < allocate.width; ++x) {
					uint8_t alpha = *sourcePixel8;
					Vec4f color = AlphaToLinear(alpha);
					if (!allocate.isPreMultiplied) {
						color.rgb *= color.a;
					}
					*destPixel32 = LinearToRGBA32(color);
					++destPixel32;
					sourcePixel8 += sourceIncrement;
				}
			}
		} break;
		case 3:
		{
			// RGB > RGBA
			size_t sourceIncrement = 3;
			size_t sourceScanline = allocate.width * 3;
			uint32_t *destPixel32 = (uint32_t *)texturePixelsRGBA;
			for (size_t y = 0; y < allocate.height; ++y) {
				uint8_t *sourceRow8 = (uint8_t *)allocate.data + (sourceScanline * (!allocate.isTopDown ? allocate.height - 1 - y : y));
				uint8_t *sourcePixel8 = sourceRow8;
				for (size_t x = 0; x < allocate.width; ++x) {
					uint8_t r = *(sourcePixel8 + 0);
					uint8_t g = *(sourcePixel8 + 1);
					uint8_t b = *(sourcePixel8 + 2);
					Pixel pixel = { r, g, b, 255 };
					Vec4f color = PixelToLinear(pixel);
					if (!allocate.isPreMultiplied) {
						color.rgb *= color.a;
					}
					*destPixel32 = LinearToRGBA32(color);
					++destPixel32;
					sourcePixel8 += sourceIncrement;
				}
			}
		} break;
		case 4:
		{
			// RBGA > RGBA
			size_t sourceIncrement = 4;
			size_t sourceScanline = allocate.width * 4;
			uint32_t *destPixel32 = (uint32_t *)texturePixelsRGBA;
			for (size_t y = 0; y < allocate.height; ++y) {
				uint32_t *sourceRow32 = (uint32_t *)allocate.data + (sourceScanline * (!allocate.isTopDown ? allocate.height - 1 - y : y));
				uint32_t *sourcePixel32 = sourceRow32;
				for (size_t x = 0; x < allocate.width; ++x) {
					uint32_t rgba = *sourcePixel32;
					Vec4f color = RGBA32ToLinear(rgba);
					if (!allocate.isPreMultiplied) {
						color.rgb *= color.a;
					}
					*destPixel32 = LinearToRGBA32(color);
					++destPixel32;
					sourcePixel32 += sourceIncrement;
				}
			}
		} break;
	}

	GLuint textureHandle;
	glGenTextures(1, &textureHandle);
	glBindTexture(GL_TEXTURE_2D, textureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, allocate.width, allocate.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texturePixelsRGBA);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glBindTexture(GL_TEXTURE_2D, 0);

	PopSize(&commandBuffer->textureData, textureSizeRGBA);

	void *result = ValueToPointer<GLuint>(textureHandle);
	*allocate.targetHandle = result;
}

static void OpenGLReleaseTexture(Render::CommandBuffer *commandBuffer, Render::TextureOperationRelease &release) {
	GLuint textureHandle = PointerToValue<GLuint>(release.handle);
	if (textureHandle > 0) {
		glDeleteTextures(1, &textureHandle);
		*release.handle = nullptr;
	}
}

inline void OpenGLCheckError() {
	GLenum error = glGetError();
	char *errorStr = nullptr;
	if (error != GL_NO_ERROR) {
		switch (error) {
			case GL_INVALID_ENUM:
				errorStr = "Invalid Enum";
				break;
			case GL_INVALID_VALUE:
				errorStr = "Invalid Value";
				break;
			case GL_INVALID_OPERATION:
				errorStr = "Invalid Operation";
				break;
			case GL_STACK_OVERFLOW:
				errorStr = "Stack Overflow";
				break;
			case GL_STACK_UNDERFLOW:
				errorStr = "Stack Underflow";
				break;
			case GL_OUT_OF_MEMORY:
				errorStr = "Out of Memory";
				break;
		}
	}
	assert(error == GL_NO_ERROR);
}

static void OpenGLDrawCommandBuffer(Render::CommandBuffer *commandBuffer) {
	// Allocate / Release textures
	commandBuffer->textureData.offset = 0;
	while (!commandBuffer->textureOperations.empty()) {
		Render::TextureOperation textureOperation = commandBuffer->textureOperations.top();
		commandBuffer->textureOperations.pop();
		switch (textureOperation.type) {
			case Render::TextureOperationType::Allocate:
			{
				OpenGLAllocateTexture(commandBuffer, textureOperation.allocate);
			} break;
			case Render::TextureOperationType::Release:
			{
				OpenGLReleaseTexture(commandBuffer, textureOperation.release);
			} break;
		}
		OpenGLCheckError();
	}

	// Render buffer
	std::stack<Render::VertexIndexArrayHeader *> vertexIndexArrayStack;
	uint8_t *commandBase = (uint8_t *)globalApp->commandBuffer->commands.base;
	uint8_t *commandAt = commandBase;
	uint8_t *commandEnd = commandAt + globalApp->commandBuffer->commands.offset;
	while (commandAt < commandEnd) {
		Render::CommandHeader *commandHeader = static_cast<Render::CommandHeader *>((void *)commandAt);
		assert(commandHeader != nullptr && commandHeader->dataSize > 0);
		commandAt += sizeof(Render::CommandHeader);

		if (!((commandHeader->type == Render::CommandType::VerticesDraw) || (commandHeader->type == Render::CommandType::IndicesDraw))) {
			OpenGLPopVertexIndexArray(vertexIndexArrayStack);
			OpenGLCheckError();
		}

		switch (commandHeader->type) {
			case Render::CommandType::Viewport:
			{
				Render::Viewport *viewport = static_cast<Render::Viewport *>((void *)commandAt);
				glViewport(viewport->x, viewport->y, viewport->w, viewport->h);
			} break;

			case Render::CommandType::OrthoProjection:
			{
				Render::OrthoProjection *ortho = static_cast<Render::OrthoProjection *>((void *)commandAt);
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glOrtho(ortho->left, ortho->right, ortho->bottom, ortho->top, ortho->nearClip, ortho->farClip);
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
			} break;

			case Render::CommandType::Clear:
			{
				Render::Clear *clear = static_cast<Render::Clear *>((void *)commandAt);
				GLbitfield flags = 0;
				if (clear->isColor) {
					flags |= GL_COLOR_BUFFER_BIT;
				}
				if (clear->isDepth) {
					flags |= GL_DEPTH_BUFFER_BIT;
				}
				glClearColor(clear->color.r, clear->color.g, clear->color.b, clear->color.a);
				glClear(flags);
			} break;

			case Render::CommandType::Lines:
			{
				Render::Vertices *lines = static_cast<Render::Vertices *>((void *)commandAt);
				Vec2f *points = lines->points;
				glColor4fv(&lines->color.m[0]);
				glLineWidth(lines->lineWidth);
				glBegin(GL_LINES);
				for (uint32_t pointIndex = 0; pointIndex < lines->pointCount; ++pointIndex) {
					Vec2f point = points[pointIndex];
					glVertex2fv(&point.m[0]);
				}
				glEnd();
				glLineWidth(1.0f);
			} break;

			case Render::CommandType::Polygon:
			{
				Render::Vertices *verts = static_cast<Render::Vertices *>((void *)commandAt);
				Vec2f *points = verts->points;
				glColor4fv(&verts->color.m[0]);
				glLineWidth(verts->lineWidth);
				glBegin(verts->isFilled ? GL_POLYGON : GL_LINE_LOOP);
				for (uint32_t pointIndex = 0; pointIndex < verts->pointCount; ++pointIndex) {
					Vec2f point = points[pointIndex];
					glVertex2fv(&point.m[0]);
				}
				glEnd();
				glLineWidth(1.0f);
			} break;

			case Render::CommandType::Rectangle:
			{
				Render::Rectangle *rect = static_cast<Render::Rectangle *>((void *)commandAt);
				glColor4fv(&rect->color.m[0]);
				glLineWidth(rect->lineWidth);
				glBegin(rect->isFilled ? GL_QUADS : GL_LINE_LOOP);
				glVertex2f(rect->bottomLeft.x + rect->size.w, rect->bottomLeft.y + rect->size.h);
				glVertex2f(rect->bottomLeft.x, rect->bottomLeft.y + rect->size.h);
				glVertex2f(rect->bottomLeft.x, rect->bottomLeft.y);
				glVertex2f(rect->bottomLeft.x + rect->size.w, rect->bottomLeft.y);
				glEnd();
				glLineWidth(1.0f);
			} break;

			case Render::CommandType::Sprite:
			{
				Render::Sprite *sprite = static_cast<Render::Sprite *>((void *)commandAt);
				Vec2f pos = sprite->position;
				Vec2f size = sprite->size;
				GLuint textureHandle = PointerToValue<GLuint>(sprite->texture);
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, textureHandle);
				glColor4fv(&sprite->color.m[0]);
				glBegin(GL_QUADS);
				glTexCoord2f(sprite->uvMax.x, sprite->uvMax.y);
				glVertex2f(pos.x + size.w, pos.y + size.h);
				glTexCoord2f(sprite->uvMin.x, sprite->uvMax.y);
				glVertex2f(pos.x, pos.y + size.h);
				glTexCoord2f(sprite->uvMin.x, sprite->uvMin.y);
				glVertex2f(pos.x, pos.y);
				glTexCoord2f(sprite->uvMax.x, sprite->uvMin.y);
				glVertex2f(pos.x + size.w, pos.y);
				glEnd();
				glBindTexture(GL_TEXTURE_2D, 0);
				glDisable(GL_TEXTURE_2D);
				glDisable(GL_BLEND);
			} break;

			case Render::CommandType::Circle:
			{
				const int segments = 16;
				const float segmentRad = ((float)M_PI * 2.0f) / (float)segments;
				Render::Circle *circle = static_cast<Render::Circle *>((void *)commandAt);
				glColor4fv(&circle->color.m[0]);
				glLineWidth(circle->lineWidth);
				glBegin(circle->isFilled ? GL_POLYGON : GL_LINE_LOOP);
				for (int segmentIndex = 0; segmentIndex < segments; ++segmentIndex) {
					float r = (float)segmentIndex * segmentRad;
					float x = circle->position.x + cosf(r) * circle->radius;
					float y = circle->position.y + sinf(r) * circle->radius;
					glVertex2f(x, y);
				}
				glEnd();
				glLineWidth(1.0f);
			} break;

			case Render::CommandType::VertexIndexHeader:
			{
				Render::VertexIndexArrayHeader *vertexIndexArray = static_cast<Render::VertexIndexArrayHeader *>((void *)commandAt);
				assert(vertexIndexArray->vertices != nullptr);
				OpenGLPushVertexIndexArray(vertexIndexArrayStack, vertexIndexArray);
			} break;

			case Render::CommandType::VerticesDraw:
			case Render::CommandType::IndicesDraw:
			{
				assert(vertexIndexArrayStack.size() > 0);
				Render::VertexIndexArrayHeader *vertexIndexArray = vertexIndexArrayStack.top();

				Render::VertexIndexArrayDraw *vertexIndexArrayDraw = static_cast<Render::VertexIndexArrayDraw *>((void *)commandAt);
				GLuint primitiveType;
				switch (vertexIndexArrayDraw->drawType) {
					case Render::PrimitiveType::Points:
					{
						primitiveType = GL_POINTS;
						float pointSize = vertexIndexArrayDraw->pointSize;
						glPointSize(pointSize);
					} break;
					case Render::PrimitiveType::Triangles:
					{
						primitiveType = GL_TRIANGLES;
					} break;
					default:
					{
						primitiveType = GL_TRIANGLES;
					} break;
				}

#if ENABLE_TEXTURES
				if (vertexIndexArrayDraw->texture != nullptr) {
					GLuint textureId = PointerToValue<GLuint>(vertexIndexArrayDraw->texture);
					glEnable(GL_BLEND);
					glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, textureId);
				}
#endif

				if (commandHeader->type == Render::CommandType::VerticesDraw) {
					glDrawArrays(primitiveType, 0, (GLsizei)vertexIndexArrayDraw->count);
				} else {

#if ENABLE_DRAWELEMENTS
					size_t indexSize = vertexIndexArray->indexSize;
					GLenum type = indexSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
					glDrawElements(primitiveType, (GLsizei)vertexIndexArrayDraw->count, type, vertexIndexArray->indices);
#else
					assert(vertexIndexArray->indices != nullptr);
					assert(vertexIndexArray->vertices != nullptr);
					assert(vertexIndexArray->texcoords != nullptr);
					assert(vertexIndexArray->colors != nullptr);
					assert(primitiveType == GL_TRIANGLES);
					uint32_t triangleCount = vertexIndexArrayDraw->count / 3;
					for (uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex) {
						uint32_t index0 = *((uint32_t *)vertexIndexArray->indices + triangleIndex * 3 + 0);
						uint32_t index1 = *((uint32_t *)vertexIndexArray->indices + triangleIndex * 3 + 1);
						uint32_t index2 = *((uint32_t *)vertexIndexArray->indices + triangleIndex * 3 + 2);

						Vec2f v0 = *(Vec2f *)((uint8_t *)vertexIndexArray->vertices + vertexIndexArray->vertexStride * index0);
						Vec2f v1 = *(Vec2f *)((uint8_t *)vertexIndexArray->vertices + vertexIndexArray->vertexStride * index1);
						Vec2f v2 = *(Vec2f *)((uint8_t *)vertexIndexArray->vertices + vertexIndexArray->vertexStride * index2);

						Vec2f t0 = *(Vec2f *)((uint8_t *)vertexIndexArray->texcoords + vertexIndexArray->texcoordStride * index0);
						Vec2f t1 = *(Vec2f *)((uint8_t *)vertexIndexArray->texcoords + vertexIndexArray->texcoordStride * index1);
						Vec2f t2 = *(Vec2f *)((uint8_t *)vertexIndexArray->texcoords + vertexIndexArray->texcoordStride * index2);

						Vec4f c0 = *(Vec4f *)((uint8_t *)vertexIndexArray->colors + vertexIndexArray->colorStride * index0);
						Vec4f c1 = *(Vec4f *)((uint8_t *)vertexIndexArray->colors + vertexIndexArray->colorStride * index1);
						Vec4f c2 = *(Vec4f *)((uint8_t *)vertexIndexArray->colors + vertexIndexArray->colorStride * index2);

						glBegin(primitiveType);
						glColor4fv(&c0.m[0]);
						glTexCoord2fv(&t0.m[0]);
						glVertex2fv(&v0.m[0]);
						glColor4fv(&c1.m[0]);
						glTexCoord2fv(&t1.m[0]);
						glVertex2fv(&v1.m[0]);
						glColor4fv(&c2.m[0]);
						glTexCoord2fv(&t2.m[0]);
						glVertex2fv(&v2.m[0]);
						glEnd();
					}
#endif

				}
#if ENABLE_TEXTURES
				if (vertexIndexArrayDraw->texture != nullptr) {
					glBindTexture(GL_TEXTURE_2D, 0);
					glDisable(GL_TEXTURE_2D);
					glDisable(GL_BLEND);
				}
#endif

				if (vertexIndexArrayDraw->drawType == Render::PrimitiveType::Points) {
					glPointSize(1.0f);
				}
			} break;
		}
		
		OpenGLCheckError();

		commandAt += commandHeader->dataSize;
	}
	OpenGLPopVertexIndexArray(vertexIndexArrayStack);
}

static void DisplayFromGLUT() {
	Render::ResetCommandBuffer(globalApp->commandBuffer);
	globalApp->UpdateAndRender(lastFrameTime, lastFrameCycles);
	OpenGLDrawCommandBuffer(globalApp->commandBuffer);

	glutSwapBuffers();

	auto endFrameClock = std::chrono::high_resolution_clock::now();
	auto durationClock = endFrameClock - lastFrameClock;
	lastFrameClock = endFrameClock;
	uint64_t frameTimeInNanos = std::chrono::duration_cast<std::chrono::nanoseconds>(durationClock).count();
	lastFrameTime = frameTimeInNanos / (float)1000000000;

	uint64_t endCycles = __rdtsc();
	lastFrameCycles = endCycles - lastCycles;
	lastCycles = endCycles;
}
static void KeyPressedFromGLUT(unsigned char key, int a, int b) {
	globalApp->KeyUp(key);
}

static void SpecialKeyPressedFromGLUT(int key, int a, int b) {
	globalApp->KeyUp(key);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	Application *app = globalApp = new DemoApplication();
	Window *window = app->GetWindow();

	int argc = 0;
	char **args = nullptr;

	glutInit(&argc, args);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(window->GetLeft(), window->GetTop());
	glutInitWindowSize(window->GetWidth(), window->GetHeight());

	std::string windowTitle = std::string("C++ NBody Simulation V") + std::string(kAppVersion);
	glutCreateWindow(windowTitle.c_str());

	glewInit();

#if _WIN32
	if (WGL_EXT_swap_control) {
		wglSwapIntervalEXT(0);
	}
#endif

	app->Init();

	glutDisplayFunc(DisplayFromGLUT);
	glutReshapeFunc(ResizeFromGLUT);
	glutIdleFunc(DisplayFromGLUT);
	glutKeyboardUpFunc(KeyPressedFromGLUT);
	glutSpecialUpFunc(SpecialKeyPressedFromGLUT);

	lastFrameClock = std::chrono::high_resolution_clock::now();
	lastFrameTime = 0.0f;
	lastCycles = __rdtsc();

	glutMainLoop();

	delete app;

	return 0;
}