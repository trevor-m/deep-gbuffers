#version 330 core
out vec4 color;

in vec2 texCoords;
in vec3 fragPos;
in vec3 normal;

uniform vec3 lightColor;

void main()
{
	color = vec4(lightColor, 1.0);
}