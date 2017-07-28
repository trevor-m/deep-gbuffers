#version 330 core
out vec3 outgoingRadiosity;
in vec2 TexCoords;

//from gbuffer
uniform sampler2DArray bufferPosition;
uniform sampler2DArray bufferNormal;
//from lighting stage or previous pass
uniform sampler2DArray bufferRadiosity;

#define NUM_LAYERS 2
#define NUM_SAMPLES 32
uniform sampler2D texNoise;
uniform vec3 samples[NUM_SAMPLES];
uniform mat4 projection;

uniform int which;

//tiling factor for noise texture
const vec2 noiseScale = vec2(1000.0/4.0, 800/4.0);

//parameters
const float radius = 2;

#define M_PI 3.1415926535897932384626433832795

void main()
{
	//get normal/position of closest layer (X)
	vec3 posX = texture(bufferPosition, vec3(TexCoords, 0)).xyz;
	vec3 normalX = normalize(texture(bufferNormal, vec3(TexCoords, 0)).xyz);

	//create matrix to rotate sample kernel a random amount using noise texture
	vec3 noise = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
	vec3 tangent = normalize(noise - normalX * dot(noise, normalX));
	vec3 bitangent = cross(normalX, tangent);
	mat3 T = mat3(tangent, bitangent, normalX);

	//take N samples from both buffers
	//but only the M which (w*nx) > 0 and (w*ny) < 0 (aka the non-zero ones)
	int M = 0;
	vec3 irradiance = vec3(0);
	for(int i = 0; i < NUM_SAMPLES; i++)
	{
		//rotate sample
		vec3 samplePos = T * samples[i];
		//add this offset to fragment position
		samplePos = posX + samplePos * radius; 

		//backproject sample to screen space
		vec4 coords = vec4(samplePos, 1.0);
		coords = projection * coords;
		coords.xyz /= coords.w;
		coords.xyz = coords.xyz * 0.5 + 0.5;

		for(int j = 0; j < NUM_LAYERS; j++) 
		{
			if(which == 1 && j == 1)
				continue;
			else if(which == 2 && j == 0)
				continue;
			//look at position in given layer at these screen coordinates (Y)
			vec3 posY = texture(bufferPosition, vec3(coords.xy, j)).xyz;
			//get outgoing radiosity from this point (B(Y))
			vec3 radiosityY = texture(bufferRadiosity, vec3(coords.xy, j)).xyz;
			//get normal of this point Ny
			vec3 normalY = texture(bufferNormal, vec3(coords.xy, j)).xyz;

			//get direction from X to Y (omega)
			vec3 direction = normalize(posY - posX);
			float facingX = dot(direction, normalX);
			float facingY = dot(direction, normalY);

			//if both samples are facing, use this sample
			int use = ((facingX > 0) && (facingY < 0)) ? 1 : 0;
			//scale by how close it's facing normalX
			radiosityY *= max(facingX, 0);
			//add radiosity from this sample
			irradiance += (use == 1) ? radiosityY : 0;
			M += use;
		}
	}
	//normalize
	irradiance *= (2 * M_PI) / M;
	irradiance = max(irradiance, 0);

	//boost saturated colors
	float maxChannel = max(irradiance.r, max(irradiance.g, irradiance.b));
	float minChannel = min(irradiance.r, min(irradiance.g, irradiance.b));
	float boost = maxChannel - minChannel / maxChannel;

	//reflectivity (can be per material)
	float reflectivity = 1;

	//convert to outgoing radiance
	outgoingRadiosity = irradiance * reflectivity * boost; 

	//confidence value
	//confidence = M / NUM_SAMPLES;
}