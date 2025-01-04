#version 460 core

typedef struct Plane
{
	vec3 normal;
	float distance;
};

typedef struct Frustum
{
	Plane top;
	Plane bottom;
	Plane left;
	Plane right;
	Plane far;
	Plane near;
};

typedef struct DrawElementsIndirectCommand
{
	uint32_t count;
	uint32_t instanceCount;
	uint32_t firstIndex;
	uint32_t baseVertex;
	uint32_t baseInstance;
};

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std420) buffer Data
{
	DrawElementsIndirectCommand commands[768];
	vec3 chunkPositions[768];
	Frustum cameraFrustum;
	vec3 chunkSize;
};

float GetSignedDistanceToPlane(Plane plane, vec3 point)
{
	return dot(plane.normal, point) - plane.distance;
}

bool IsOnOrForwardPlane(Plane plane, vec3 pos, vec3 scale)
{
	// Compute the projection interval radius of b onto L(t) = b.c + t * p.n
	float r = scale.x * abs(plane.normal.x) +
		scale.y * abs(plane.normal.y) + scale.z * abs(plane.normal.z);

	return -r <= GetSignedDistanceToPlane(plane, pos);
}


bool IsInFrustum(uint index)
{
	//Get global scale thanks to our transform
	vec3 globalCenter{  };

	// Scaled orientation
	vec3 right = transform.getRight() * extents.x;
	vec3 up = transform.getUp() * extents.y;
	vec3 forward = transform.getForward() * extents.z;

	float newIi = abs(dot(vec3( 1.f, 0.f, 0.f ), right)) +
		abs(dot(vec3( 1.f, 0.f, 0.f ), up)) +
		abs(dot(vec3( 1.f, 0.f, 0.f ), forward));

	float newIj = abs(dot(vec3( 0.f, 1.f, 0.f ), right)) +
		abs(dot(vec3( 0.f, 1.f, 0.f ), up)) +
		abs(dot(vec3( 0.f, 1.f, 0.f ), forward));

	float newIk = abs(dot(vec3( 0.f, 0.f, 1.f), right)) +
		abs(dot(vec3( 0.f, 0.f, 1.f ), up)) +
		abs(dot(vec3( 0.f, 0.f, 1.f ), forward));

	vec3 pos = transform.getModelMatrix() * vec4(center, 1.f);
	vec3 scale = vec3(newIi, newIj, newIk);

	//We not need to divise scale because it's based on the half extention of the AABB
	AABB globalAABB(globalCenter, newIi, newIj, newIk);

	return (IsOnOrForwardPlane(cameraFrustum.left, pos, scale) &&
		IsOnOrForwardPlane(cameraFrustum.right, pos, scale) &&
		IsOnOrForwardPlane(cameraFrustum.top, pos, scale) &&
		IsOnOrForwardPlane(cameraFrustum.bottom, pos, scale) &&
		IsOnOrForwardPlane(cameraFrustum.near, pos, scale) &&
		IsOnOrForwardPlane(cameraFrustum.far, pos, scale));
}

void ClearDrawCommand()
{
	commands[gl_LocalInvocationIndex].count = 0;
	commands[gl_LocalInvocationIndex].instanceCount = 0;
	commands[gl_LocalInvocationIndex].firstIndex = 0;
	commands[gl_LocalInvocationIndex].baseVertex = 0;
	commands[gl_LocalInvocationIndex].baseInstance = 0;
}

void main()
{
	if (!IsInFrustum(gl_LocalInvocationIndex))
	{
		ClearDrawCommand();
	}
}