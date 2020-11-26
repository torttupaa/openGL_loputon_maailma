#shader vertex
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

uniform mat4 model;

out vec2 TexCoords;

void main()
{
    gl_Position = model * vec4(position, 1.0);
    TexCoords = uv;
}

#shader fragment
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D tex;

void main()
{
    FragColor = texture(tex, TexCoords);
}