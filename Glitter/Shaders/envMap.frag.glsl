#version 330 core
in vec3 TexCoords;
out vec4 color;

uniform samplerCube envMap;

void main()
{    
    color = texture(envMap, TexCoords);
}