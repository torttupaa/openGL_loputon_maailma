#shader vertex
#version 330 core

layout(location = 0) in vec3 aPos;
//layout(location = 1) in vec2 aUV;

uniform mat4 gworld;

//out vec2 tex_cord;


void main()
{
	gl_Position = gworld * vec4(aPos, 1.0f);
	//tex_cord = aUV;
};



#shader fragment
#version 330 core

//in vec2 tex_cord;

//uniform sampler2D tex;

out vec4 outColor;

void main()
{
	//outColor = texture2D(tex, tex_cord);
	outColor = vec4(1.0f,0.0f,0.0f,1.0f);
};