#shader vertex
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in mat4 instanceMatrix;

vec3 Normal = vec3(0.0, 1.0, 0.0);

out vec2 TexCoord_CS_in;
out vec3 WorldPos_CS_in;
out vec3 Normal_CS_in;

void main()
{
	WorldPos_CS_in = vec3(instanceMatrix * vec4(aPos, 1.0f));
	TexCoord_CS_in = aUV;
	Normal_CS_in = normalize((instanceMatrix * vec4(aNormal, 0.0f)).xyz);
	//Normal_CS_in = aNormal;
};


#shader tcs
#version 460 core

layout(vertices = 3) out;

uniform vec3 gEyeWorldPos;

in vec3 WorldPos_CS_in[];
in vec2 TexCoord_CS_in[];
in vec3 Normal_CS_in[];

out vec3 WorldPos_ES_in[];
out vec2 TexCoord_ES_in[];
out vec3 Normal_ES_in[];

float GetTessLevel(float Distance1)
{
	if (Distance1 <= 250.0) {
		return 32;
	}
	else {
		return 1;
	}
}

void main()
{
	TexCoord_ES_in[gl_InvocationID] = TexCoord_CS_in[gl_InvocationID];
	Normal_ES_in[gl_InvocationID] = Normal_CS_in[gl_InvocationID];
	WorldPos_ES_in[gl_InvocationID] = WorldPos_CS_in[gl_InvocationID];
	float EyeToVertexDistance2 = distance(gEyeWorldPos, WorldPos_ES_in[2]);

	gl_TessLevelOuter[0] = GetTessLevel(EyeToVertexDistance2);
	gl_TessLevelOuter[1] = gl_TessLevelOuter[0];
	gl_TessLevelOuter[2] = gl_TessLevelOuter[0];
	gl_TessLevelInner[0] = gl_TessLevelOuter[2];
}


#shader tes
#version 460 core

layout(triangles, equal_spacing, ccw) in;

uniform mat4 PV;

uniform sampler2D gDisplacementMap;

float gDispFactor = 1.0f;
const float offset = 1.0 / 1024;

in vec3 WorldPos_ES_in[];
in vec2 TexCoord_ES_in[];
in vec3 Normal_ES_in[];

out vec3 FragPos;
out vec2 tex_cord;
out vec3 Normal;


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
	Normal = interpolate3D(Normal_ES_in[0], Normal_ES_in[1], Normal_ES_in[2]);
	FragPos = interpolate3D(WorldPos_ES_in[0], WorldPos_ES_in[1], WorldPos_ES_in[2]);

	//Normal = normalize(Normal);
	float Displacement = (texture(gDisplacementMap, tex_cord.xy).x)*2-1;
	FragPos += (Normal * Displacement * gDispFactor);
	gl_Position = PV * vec4(FragPos, 1.0);

	//calculate from bumb

	vec2 offsets[4] = vec2[](
		vec2(0.0f, offset), // top-center
		vec2(-offset, 0.0f),   // center-left
		vec2(offset, 0.0f),   // center-right
		vec2(0.0f, -offset) // bottom-center  
		);


	
	Normal[0] -= (Displacement - (texture(gDisplacementMap, tex_cord.xy + offsets[0]).xyz).x) * 100;
	Normal[0] += (Displacement - (texture(gDisplacementMap, tex_cord.xy + offsets[3]).xyz).x) * 100;


	Normal[2] -= (Displacement - (texture(gDisplacementMap, tex_cord.xy + offsets[1]).xyz).x) * 100;
	Normal[2] += (Displacement - (texture(gDisplacementMap, tex_cord.xy + offsets[2]).xyz).x) * 100;
	
}


#shader fragment
#version 460 core

in vec2 tex_cord;
in vec3 Normal;
in vec3 FragPos;

float diff;
vec3 diffuse;
vec4 texel;
vec3 norm;
vec3 lightDir;
vec3 ambient = vec3(0.0f, 0.0f, 0.0f);
const vec4 FogColor = vec4(1, 1, 1, 1.0);

//uniform vec3 ambient;
uniform sampler2D tex;
uniform vec3 gEyeWorldPos;

out vec4 outColor;

float getFogFactor(float d)
{
	const float FogMax = 5000.0;
	const float FogMin = 3000.0;

	if (d >= FogMax) return 1;
	if (d <= FogMin) return 0;

	return 1 - (FogMax - d) / (FogMax - FogMin);
}

void main()
{
	vec4 texel = texture2D(tex, tex_cord);
	if (texel.a < 0.1)
		discard;


	// compute tangent T and bitangent B
	vec3 Q1 = dFdx(FragPos);
	vec3 Q2 = dFdy(FragPos);
	vec2 st1 = dFdx(tex_cord);
	vec2 st2 = dFdy(tex_cord);

	vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
	vec3 B = normalize(-Q1 * st2.s + Q2 * st1.s);
	vec3 N = normalize(Normal);

	// the transpose of texture-to-eye space matrix
	mat3 TBN = mat3(T, B, N);



	//norm = normalize(TBN * Normal);
	norm = -N;

	//lightDir = normalize(lightpos - FragPos);
	lightDir = normalize(vec3(-1.0, 1.0, -1.0));

	diff = max(dot(norm, lightDir), 0.0f);
	diffuse = diff * vec3(1.0f, 1.0f, 1.0f);

	//texel = texture2D(tex, tex_cord);

	outColor = vec4((diffuse + ambient), 1.0) * texel;

	float distance = distance(FragPos, gEyeWorldPos);
	float alpha = getFogFactor(distance);
	outColor = mix(outColor, FogColor, alpha);
	//outColor = vec4(norm,1.0f);


};