#shader vertex
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in mat4 instanceMatrix;

uniform mat4 PV;


out vec2 tex_cord;
out vec3 FragPos;
out vec3 Normal;


void main()
{
	gl_Position = PV * instanceMatrix * vec4(aPos, 1.0f);
	FragPos = vec3(instanceMatrix * vec4(aPos, 1.0f));
	tex_cord = aUV;
	Normal = (instanceMatrix * vec4(aNormal, 0.0)).xyz;
};



#shader fragment
#version 330 core

in vec2 tex_cord;
in vec3 Normal;
in vec3 FragPos;


const vec4 FogColor = vec4(1.0, 1.0, 1.0, 1.0);
vec3 ambient = vec3(0.4, 0.4, 0.4);

uniform sampler2D tex;
uniform sampler2D normalMap;
uniform vec3 gEyeWorldPos;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outColor;


void main()
{
	outPosition = FragPos;

	vec4 texel = texture2D(tex, tex_cord);
	if (texel.a < 0.1)
		discard;
	outColor = texel.xyz;

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
	vec3 Normal = normalize(texture(normalMap, tex_cord).xyz * 2.0 - 1.0);
	outNormal = normalize(TBN * Normal);

};