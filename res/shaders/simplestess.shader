#shader vertex
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

//vec3 Normal = vec3(0.0,1.0,0.0);
uniform mat4 model;

out vec2 TexCoord_CS_in;
out vec3 WorldPos_CS_in;
out vec3 Normal_CS_in;

void main()
{
	WorldPos_CS_in = vec3(model * vec4(aPos, 1.0f));
	TexCoord_CS_in = aUV;
	//Normal_CS_in = Normal;
};


#shader tcs
#version 460 core

layout(vertices = 3) out;

uniform vec3 gEyeWorldPos;

in vec3 WorldPos_CS_in[];
in vec2 TexCoord_CS_in[];
//in vec3 Normal_CS_in[];

out vec3 WorldPos_ES_in[];
out vec2 TexCoord_ES_in[];
out vec3 Normal_ES_in[];

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
#version 460 core

layout(triangles, equal_spacing, ccw) in;

uniform mat4 PV;

uniform sampler2D gDisplacementMap;
uniform sampler2D normalMap;
uniform mat4 model;

float gDispFactor = 700.0f;
const float offset = 1.0 / 512;
vec3 Tangent = vec3(1.0, 0.0, 0.0);
vec3 Bitangent = vec3(0.0,0.0,1.0);


in vec3 WorldPos_ES_in[];
in vec2 TexCoord_ES_in[];
//in vec3 Normal_ES_in[];

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
	//Normal = interpolate3D(Normal_ES_in[0], Normal_ES_in[1], Normal_ES_in[2]);
	FragPos = interpolate3D(WorldPos_ES_in[0], WorldPos_ES_in[1], WorldPos_ES_in[2]);

	Normal = vec3(0.0,1.0,0.0);
	//vec4 asd = textureGather(gDisplacementMap, tex_cord);
	//float Displacement = (asd[0] + asd[1] + asd[2] + asd[3])/4;

	//float Displacement = texture(gDisplacementMap, tex_cord.xy).x;
	//FragPos += (Normal * Displacement * gDispFactor);
	//gl_Position = PV * vec4(FragPos, 1.0);

	//calculate from bumb
	
	vec2 offsets[4] = vec2[](
		vec2(0.0f, offset), // top-center
		vec2(-offset, 0.0f),   // center-left
		vec2(offset, 0.0f),   // center-right
		vec2(0.0f, -offset) // bottom-center  
		);

	float Displacement = texture(gDisplacementMap, tex_cord.xy).x;
	for (int i = 0; i < 4; i++)
	{
		Displacement+= texture(gDisplacementMap, tex_cord.xy+ offsets[i]).x;
	}
	Displacement = Displacement / 5;
	FragPos += (Normal * Displacement * gDispFactor);
	gl_Position = PV * vec4(FragPos, 1.0);
	
	//calculate from normal map
	

	Normal = (model * vec4(Normal, 0.0)).xyz;
	/*
	Normal = normalize(Normal);
	Tangent = normalize(Tangent);
	Bitangent = normalize(Bitangent);
	Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
	mat3 TBN = mat3(Tangent, Bitangent, Normal);
	*/
	//for normal smoothening
	/*
	vec3 BumpMapNormal = texture(normalMap, tex_cord).xyz;
	for (int i = 0; i < 4; i++)
	{
		BumpMapNormal += ((texture(normalMap, tex_cord.xy + offsets[i]).xyz)*2.0 - vec3(1.0, 1.0, 1.0));
	}


	Normal = normalize(TBN * BumpMapNormal);*/

	Normal[0] -= (Displacement - (texture(gDisplacementMap, tex_cord.xy+ offsets[0]).xyz).x)* 50;
	Normal[0] += (Displacement - (texture(gDisplacementMap, tex_cord.xy + offsets[3]).xyz).x)* 50;


	Normal[2] -= (Displacement - (texture(gDisplacementMap, tex_cord.xy + offsets[1]).xyz).x)* 50;
	Normal[2] += (Displacement - (texture(gDisplacementMap, tex_cord.xy + offsets[2]).xyz).x)* 50;
	//Normal = normalize(TBN * normalize(Normal));

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
vec3 ambient = vec3(0.0f,0.0f,0.0f);
const vec4 FogColor = vec4(1, 1, 1,1.0);

//uniform vec3 ambient;
uniform vec3 lightpos;
uniform sampler2D tex;
uniform sampler2D rocktext;
uniform sampler2D rocknormal;
uniform sampler2D normalMap2;
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
float getRockFactor(vec3 normal)
{
	float rockfactor = dot(normal, vec3(0.0, 1.0, 0.0)) * 4 - 3;
	if (rockfactor <= 0)
	{
		return 0.0;
	}
	else
	{
		return rockfactor;
	}
	return 0.0;
}

void main()
{
	vec3 Q1 = dFdx(FragPos);
	vec3 Q2 = dFdy(FragPos);
	vec2 st1 = dFdx(tex_cord);
	vec2 st2 = dFdy(tex_cord);
	vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
	vec3 B = normalize(-Q1 * st2.s + Q2 * st1.s);
	vec3 N = normalize(Normal);
	mat3 TBN = mat3(T, B, N);

	float rockfactor = getRockFactor(N); //tata ei sit oo implementoitu deferreddiin via lol

		///NORMALS TO WORLDSPACE
	vec3 Normal = normalize(texture(normalMap2, tex_cord * 80).xyz * 2.0 - 1.0);
	vec3 Normal2 = normalize(texture(rocknormal, tex_cord * 80).xyz * 2.0 - 1.0);
	Normal = mix(Normal2, Normal, rockfactor);
	norm = normalize(TBN * Normal);
	//norm = N;

	//lightDir = normalize(lightpos - FragPos);
	lightDir = normalize(vec3(-1.0,1.0,-1.0));

	diff = max(dot(norm, lightDir), 0.0f);
	diffuse = diff * vec3(1.0f, 1.0f, 1.0f);

	texel = texture2D(tex, tex_cord * 80);
	vec4 texel2 = texture2D(rocktext, tex_cord * 80);
	texel = mix(texel2, texel, rockfactor);

	outColor = vec4((diffuse + ambient), 1.0) * texel;

	float distance = distance(FragPos, gEyeWorldPos);
	float alpha = getFogFactor(distance);
	outColor = mix(outColor, FogColor, alpha);
	
	//outColor = mix(outColor, FogColor, alpha);
	
};