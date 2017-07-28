#version 330 core
out float color;
in vec2 TexCoords;

uniform sampler2DArray bufferPosition;
uniform sampler2DArray bufferNormal;
uniform sampler2D texNoise;

#define NUM_SAMPLES 32
uniform vec3 samples[NUM_SAMPLES];
uniform mat4 projection;

//tiling factor for noise texture
const vec2 noiseScale = vec2(1000.0/4.0, 800/4.0);

//parameters
const float radius = 2;
const float beta = 0.05;
const float epsilon = 0.000001;


#define M_PI 3.1415926535897932384626433832795

float aoLayer(int i, int j, mat3 T, vec3 fragPos, vec3 fragNormal) {
	//rotate sample
	vec3 samplePos = T * samples[i];
	//add this offset to fragment position
	samplePos = fragPos + samplePos * radius; 

	//backproject sample to screen space
	vec4 coords = vec4(samplePos, 1.0);
	coords = projection * coords;
	coords.xyz /= coords.w;
	coords.xyz = coords.xyz * 0.5 + 0.5;

	//look at position in given layer at these screen coordinates (Y)
	vec3 layerPos = texture(bufferPosition, vec3(coords.xy, j)).xyz;

	//v = Y - X
	vec3 v = layerPos - fragPos;
	//from 3.1 (3)
	return (1 - dot(v, v)/(radius * radius)) * max(0,(dot(v, fragNormal) - beta)/sqrt(dot(v,v) + epsilon));
}

void main()
{
	//get normal/position of closest layer (X)
	vec3 fragPos = texture(bufferPosition, vec3(TexCoords, 0)).xyz;
	vec3 normal = normalize(texture(bufferNormal, vec3(TexCoords, 0)).xyz);

	//create matrix to rotate sample kernel a random amount using noise texture
	vec3 noise = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
	vec3 tangent = normalize(noise - normal * dot(noise, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 T = mat3(tangent, bitangent, normal);

	//take samples
	float occlusion = 0.0;
	for(int i = 0; i < NUM_SAMPLES; i++)
	{
		//use whichever layer gives highest value (aka closest)
		occlusion += max(0, max(aoLayer(i, 0, T, fragPos, normal), aoLayer(i, 1, T, fragPos, normal)));
	}
	//normalize occlusion factor and convert to ambient visibility
	color = max(0, 1 - sqrt(occlusion * M_PI / NUM_SAMPLES));
}