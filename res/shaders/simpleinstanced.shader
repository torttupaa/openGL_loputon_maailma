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
	//Normal = mat3(TI_model) * aNormal;
	Normal = aNormal;
};



#shader fragment
#version 330 core

in vec2 tex_cord;
in vec3 Normal;
in vec3 FragPos;


const vec4 FogColor = vec4(1.0, 1.0, 1.0, 1.0);
vec3 ambient = vec3(0.4,0.4,0.4);

//uniform vec3 ambient;
//uniform vec3 lightpos;
uniform sampler2D tex;
uniform sampler2D normalMap;
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
	vec3 Normal = normalize(texture(normalMap, tex_cord).xyz * 2.0 - 1.0);
	vec3 norm = normalize(TBN * Normal);

	//lightDir = normalize(lightpos - FragPos);
	vec3 lightDir = normalize(vec3(-1.0,1.0,-1.0));

	float diff = max(dot(norm, lightDir), 0.0f);
	float diff2 = max(dot(-norm, lightDir), 0.0f);
	vec3 diffuse = diff * vec3(1.0f, 1.0f, 1.0f);
	vec3 diffuse2 = diff2 * vec3(1.0f, 1.0f, 1.0f); //toi on valon vari toi vektori tossa

	vec4 O1 = vec4((diffuse + ambient), 1.0);
	vec4 O2 = vec4((diffuse2 + ambient), 1.0);
	outColor = mix(O1,O2,0.5) * texel;

	float distance = distance(FragPos, gEyeWorldPos);
	float alpha = getFogFactor(distance);
	outColor = mix(outColor, FogColor, alpha);

};