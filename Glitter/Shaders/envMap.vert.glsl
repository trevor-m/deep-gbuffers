#version 330 core
layout (location = 0) in vec3 position;
out vec3 TexCoords;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;


void main()
{
    vec4 pos = projection * view * model * vec4(position, 1.0);
	//set at depth = w so that it is always at max depth
	gl_Position = pos.xyww;
    TexCoords = position;
}  