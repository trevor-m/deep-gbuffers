#version 330 core
layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoords;

out vec3 fragPos;
out vec2 texCoords;
out vec3 normal;
out vec4 clipSpaceCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 viewPos = view * model * vec4(Position, 1.0f);
	fragPos = vec3(viewPos);
    clipSpaceCoords = projection * viewPos;
	gl_Position = clipSpaceCoords;
    texCoords = TexCoords;
    mat3 normalMatrix = transpose(inverse(mat3(view * model)));
    normal = normalMatrix * Normal;
}