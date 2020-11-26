#shader vertex
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
out vec2 TexCoords;

void main()
{
    gl_Position = vec4(position, 1.0);
    TexCoords = uv;
}

#shader fragment
#version 330 core


out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D Pos;
uniform sampler2D Norm;
uniform sampler2D Color;
uniform sampler2D shadowmap;

uniform vec3 gEyeWorldPos;
uniform mat4 lightProjection;
uniform mat4 lightView;
uniform mat4 cameraView;

const vec4 FogColor = vec4(1, 1, 1, 1.0);
vec3 lightDir = normalize(vec3(-1.0, 1.0, -1.0));
vec3 ambient = vec3(0.0f, 0.0f, 0.0f);

float getFogFactor(float d)
{
	const float FogMax = 5000.0;
	const float FogMin = 3000.0;

	if (d >= FogMax) return 1;
	if (d <= FogMin) return 0;

	return 1 - (FogMax - d) / (FogMax - FogMin);
}


float shadowmapread(vec3 eyedir)
{
    mat4 cameraViewToWorldMatrix = inverse(cameraView);
    mat4 cameraViewToProjectedLightSpace = lightProjection * lightView * cameraViewToWorldMatrix;
    vec4 projectedEyeDir = cameraViewToProjectedLightSpace * vec4(eyedir, 1.0f);
    projectedEyeDir = projectedEyeDir / projectedEyeDir.w;

    vec2 textureCoordinates = projectedEyeDir.xy * vec2(0.5, 0.5) + vec2(0.5, 0.5);

    const float bias = 0.0001;
    float depthValue = texture2D(shadowmap, textureCoordinates).x;
    float currentDepth = projectedEyeDir.z;

    return currentDepth > depthValue ? 1.0 : 0.0;
}

void main()
{
    vec3 normal = normalize(texture(Norm, TexCoords).xyz);
    vec3 FragPos = texture(Pos, TexCoords).xyz;
    vec4 texel = texture(Color, TexCoords);


	float diff = max(dot(normal, lightDir), 0.0f);
	vec3 diffuse = diff * vec3(1.0f, 1.0f, 1.0f);
	FragColor = vec4((diffuse + ambient), 1.0) * texel;

	float distance = distance(FragPos, gEyeWorldPos);
	float alpha = getFogFactor(distance);

    vec3 eyeDir = FragPos - gEyeWorldPos;

    //float shadow = shadowmapread(eyeDir);

	//FragColor = mix((FragColor*(1.0-shadow)), FogColor, alpha);
    FragColor = mix(FragColor, FogColor, alpha);
}