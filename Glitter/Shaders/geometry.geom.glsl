#version 430 core
layout (triangles) in;
layout (triangle_strip, max_vertices=6) out;

in vec3 fragPos[];
in vec2 texCoords[];
in vec3 normal[];
in vec4 clipSpaceCoords[];

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;
out vec4 ClipSpaceCoords;

#define GBUFFER_LAYERS 2

void main()
{
	//send triangle to all layers
	for(int layer = 0; layer < GBUFFER_LAYERS; layer++) {
		gl_Layer = layer;
		for(int i = 0; i < 3; i++)
		{
			FragPos = fragPos[i];
			TexCoords = texCoords[i];
			Normal = normal[i];
			ClipSpaceCoords = clipSpaceCoords[i];
		
			gl_Position = gl_in[i].gl_Position;
			EmitVertex();
		}    
		EndPrimitive();
	}
}  