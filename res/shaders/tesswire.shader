#shader vertex
#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

uniform mat4 model;

out vec2 TexCoord_CS_in;
out vec3 WorldPos_CS_in;

void main()
{
	WorldPos_CS_in = vec3(model * vec4(aPos, 1.0f));
	TexCoord_CS_in = aUV;
};


#shader tcs
#version 410 core

layout(vertices = 3) out;

uniform vec3 gEyeWorldPos;

in vec3 WorldPos_CS_in[];
in vec2 TexCoord_CS_in[];

out vec3 WorldPos_ES_in[];
out vec2 TexCoord_ES_in[];

float GetTessLevel(float Distance0, float Distance1)
{
	float AvgDistance = (Distance0 + Distance1) / 2.0;

	if (AvgDistance <= 500) {
		return 16;
	}
	else if (AvgDistance <= 1000.0) {
		return 8;
	}
	else if (AvgDistance <= 2000.0) {
		return 4;
	}
	else if (AvgDistance <= 3000.0) {
		return 2;
	}
	else {
		return 1;
	}
}

void main()
{

	TexCoord_ES_in[gl_InvocationID] = TexCoord_CS_in[gl_InvocationID];
	//Normal_ES_in[gl_InvocationID] = Normal_CS_in[gl_InvocationID];
	WorldPos_ES_in[gl_InvocationID] = WorldPos_CS_in[gl_InvocationID];
	// Calculate the distance from the camera to the three control points
	//float EyeToVertexDistance0 = distance(gEyeWorldPos, WorldPos_ES_in[0]);
	//float EyeToVertexDistance1 = distance(gEyeWorldPos, WorldPos_ES_in[1]);
	float EyeToVertexDistance1 = 0.0f;
	float EyeToVertexDistance2 = distance(gEyeWorldPos, WorldPos_ES_in[2]);

	// Calculate the tessellation levels
	gl_TessLevelOuter[0] = GetTessLevel(EyeToVertexDistance1, EyeToVertexDistance2);
	//gl_TessLevelOuter[1] = GetTessLevel(EyeToVertexDistance2, EyeToVertexDistance0);
	//gl_TessLevelOuter[2] = GetTessLevel(EyeToVertexDistance0, EyeToVertexDistance1);
	gl_TessLevelOuter[1] = gl_TessLevelOuter[0];
	gl_TessLevelOuter[2] = gl_TessLevelOuter[0];
	gl_TessLevelInner[0] = gl_TessLevelOuter[2];
}


#shader tes
#version 410 core

layout(triangles, equal_spacing, ccw) in;

uniform mat4 PV;

uniform sampler2D gDisplacementMap;
uniform sampler2D tex;
float gDispFactor = 700.0f;
const float offset = 1.0 / 2048.0;
vec3 Normal;

in vec3 WorldPos_ES_in[];
in vec2 TexCoord_ES_in[];

out vec3 FragPos;
out vec2 tex_cord;

vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2)
{
	return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
}
vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2)
{
	return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

void main()
{
	tex_cord = interpolate2D(TexCoord_ES_in[0], TexCoord_ES_in[1], TexCoord_ES_in[2]);
	FragPos = interpolate3D(WorldPos_ES_in[0], WorldPos_ES_in[1], WorldPos_ES_in[2]);

	Normal = vec3(0.0, 1.0, 0.0);
	// Displace the vertex along the normal
	vec4 asd = textureGather(gDisplacementMap, tex_cord);
	float Displacement = (asd[0] + asd[1] + asd[2] + asd[3]) / 4;


	//float Displacement = texture(gDisplacementMap, tex_cord.xy).x;
	FragPos += (Normal * Displacement * gDispFactor);
	gl_Position = PV * vec4(FragPos, 1.0);
}


#shader fragment
#version 410 core

in vec3 FragPos;

out vec4 outColor;

void main()
{
	outColor = vec4(0.0,1.0,0.0, 1.0);
};