#version 430 core
layout (triangles) in;
layout (triangle_strip, max_vertices=6) out;

in vec2 texCoords[];

out vec2 TexCoords;

#define GBUFFER_LAYERS 2

void main()
{
	//send triangle to all layers
	for(int layer = 0; layer < GBUFFER_LAYERS; layer++) {
		gl_Layer = layer;
		for(int i = 0; i < 3; i++)
		{
			TexCoords = texCoords[i];
			gl_Position = gl_in[i].gl_Position;
			EmitVertex();
		}    
		EndPrimitive();
	}
}  