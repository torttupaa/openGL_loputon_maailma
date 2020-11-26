#shader vertex
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
uniform mat4 TI_model;

out vec2 tex_cord;
out vec3 FragPos;
out vec3 Normal;


void main()
{
	gl_Position = projection * view  * model * vec4(aPos, 1.0f);
	FragPos = vec3(model * vec4(aPos, 1.0f));
	tex_cord = aUV;
	Normal = mat3(TI_model) * aNormal;
};



#shader fragment
#version 330 core

in vec2 tex_cord;
in vec3 Normal;
in vec3 FragPos;

float diff;
vec3 diffuse;
vec4 texel;
vec3 norm;
vec3 lightDir;

uniform vec3 ambient;
uniform vec3 lightpos;
uniform sampler2D tex;

out vec4 outColor;

void main()
{
	norm = normalize(Normal);
	lightDir = normalize(lightpos - FragPos);

	diff = max(dot(norm, lightDir), 0.0f);
	diffuse = diff * vec3(1.0f,1.0f,1.0f);

	texel = texture2D(tex, tex_cord);
	outColor = vec4((diffuse+ambient), 1.0) * texel;
};