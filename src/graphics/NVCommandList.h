#pragma once

#include <stdint.h>
#include <glad/glad.h>

typedef struct {
  uint32_t  header;
} TerminateSequenceCommandNV;

typedef struct {
  uint32_t  header;
} NOPCommandNV;

typedef  struct {
  uint32_t  header;
  uint32_t  count;
  uint32_t  firstIndex;
  uint32_t  baseVertex;
} DrawElementsCommandNV;

typedef  struct {
  uint32_t  header;
  uint32_t  count;
  uint32_t  first;
} DrawArraysCommandNV;

typedef  struct {
  uint32_t  header;
  uint32_t  mode;
  uint32_t  count;
  uint32_t  instanceCount;
  uint32_t  firstIndex;
  uint32_t  baseVertex;
  uint32_t  baseInstance;
} DrawElementsInstancedCommandNV;

typedef  struct {
  uint32_t  header;
  uint32_t  mode;
  uint32_t  count;
  uint32_t  instanceCount;
  uint32_t  first;
  uint32_t  baseInstance;
} DrawArraysInstancedCommandNV;

typedef struct {
  uint32_t  header;
  uint32_t  addressLo;
  uint32_t  addressHi;
  uint32_t  typeSizeInByte;
} ElementAddressCommandNV;

typedef struct {
  uint32_t  header;
  uint32_t  index;
  uint32_t  addressLo;
  uint32_t  addressHi;
} AttributeAddressCommandNV;

typedef struct {
  uint32_t    header;
  ushort  index;
  ushort  stage;
  uint32_t    addressLo;
  uint32_t    addressHi;
} UniformAddressCommandNV;

typedef struct {
  uint32_t  header;
  float red;
  float green;
  float blue;
  float alpha;
} BlendColorCommandNV;

typedef struct {
  uint32_t  header;
  uint32_t  frontStencilRef;
  uint32_t  backStencilRef;
} StencilRefCommandNV;

typedef struct {
  uint32_t  header;
  float lineWidth;
} LineWidthCommandNV;

typedef struct {
  uint32_t  header;
  float scale;
  float bias;
} PolygonOffsetCommandNV;

typedef struct {
  uint32_t  header;
  float alphaRef;
} AlphaRefCommandNV;

typedef struct {
  uint32_t  header;
  uint32_t  x;
  uint32_t  y;
  uint32_t  width;
  uint32_t  height;
} ViewportCommandNV; // only ViewportIndex 0

typedef struct {
  uint32_t  header;
  uint32_t  x;
  uint32_t  y;
  uint32_t  width;
  uint32_t  height;
} ScissorCommandNV; // only ViewportIndex 0

typedef struct {
  uint32_t  header;
  uint32_t  frontFace; // 0 for CW, 1 for CCW
} FrontFaceCommandNV;

class NVCommandList
{
public:

  NVCommandList()
  {
    glCreateCommandListsNV(1, &ID);
  }
  ~NVCommandList()
  {
    glDeleteCommandListsNV(1, &ID);
  }

  GLuint ID;
};