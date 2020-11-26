#shader vertex
#version 460 core

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
#version 460 core

layout(vertices = 3) out;

uniform vec3 gEyeWorldPos;

in vec3 WorldPos_CS_in[];
in vec2 TexCoord_CS_in[];

out vec3 WorldPos_ES_in[];
out vec2 TexCoord_ES_in[];

float GetTessLevel(float Distance0, float Distance1)
{
	float AvgDistance = (Distance0 + Distance1) / 2.0;
	//y = 62,416e^-0,002x y = 115,74e-0,002x


	if (AvgDistance <= 500.0) {
		return 32;
	}
	else if (AvgDistance <= 750.0) {
		return 16;
	}
	else if (AvgDistance <= 1000.0) {
		return 8;
	}
	else if (AvgDistance <= 2000.0) {
		return 2;
	}
	else {
		return 1;
	}
	//return float(115.74 * exp(-0.002 * AvgDistance));

}

void main()
{
	TexCoord_ES_in[gl_InvocationID] = TexCoord_CS_in[gl_InvocationID];
	WorldPos_ES_in[gl_InvocationID] = WorldPos_CS_in[gl_InvocationID];
	float EyeToVertexDistance1 = 0.0f;
	float EyeToVertexDistance2 = distance(gEyeWorldPos, WorldPos_ES_in[2]);

	gl_TessLevelOuter[0] = GetTessLevel(EyeToVertexDistance1, EyeToVertexDistance2);
	gl_TessLevelOuter[1] = gl_TessLevelOuter[0];
	gl_TessLevelOuter[2] = gl_TessLevelOuter[0];
	gl_TessLevelInner[0] = gl_TessLevelOuter[2];
}


#shader tes
#version 460 core

layout(triangles, equal_spacing, ccw) in;

uniform mat4 PV;
uniform sampler2D gDisplacementMap;

float gDispFactor = 700.0f;
const float offset = 1.0 / 512;

in vec3 WorldPos_ES_in[];
in vec2 TexCoord_ES_in[];


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
	vec2 tex_cord = interpolate2D(TexCoord_ES_in[0], TexCoord_ES_in[1], TexCoord_ES_in[2]);
	vec3 FragPos = interpolate3D(WorldPos_ES_in[0], WorldPos_ES_in[1], WorldPos_ES_in[2]);

	vec3 Normal = vec3(0.0, 1.0, 0.0);

	vec2 offsets[4] = vec2[](
		vec2(0.0f, offset), // top-center
		vec2(-offset, 0.0f),   // center-left
		vec2(offset, 0.0f),   // center-right
		vec2(0.0f, -offset) // bottom-center  
		);

	float Displacement = texture(gDisplacementMap, tex_cord.xy).x;
	for (int i = 0; i < 4; i++)
	{
		Displacement += texture(gDisplacementMap, tex_cord.xy + offsets[i]).x;
	}
	Displacement = Displacement / 5;
	FragPos += (Normal * Displacement * gDispFactor);
	gl_Position = PV * vec4(FragPos, 1.0);

}


#shader fragment
#version 460 core

void main()
{

};