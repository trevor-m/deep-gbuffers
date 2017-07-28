#version 430 core
layout (location = 0) out vec3 positionBuffer;
layout (location = 1) out vec3 normalBuffer;
layout (location = 2) out vec4 colorBuffer;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec4 ClipSpaceCoords;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;

uniform sampler2DArray bufferDepthCompare;

//minimum separation between layers (in world space units)
#define MINIMUM_SEPARATION 1

//for linearizing/delinearizing depth to apply minimum separation
uniform float nearPlane = 0.1;
uniform float farPlane = 1000;

void main()
{
	if(gl_Layer == 0) {
		positionBuffer = FragPos;
		normalBuffer = normalize(Normal);
		colorBuffer.rgb = texture(texture_diffuse1, TexCoords).rgb;
		colorBuffer.a = texture(texture_specular1, TexCoords).r;
	}
	else if(gl_Layer == 1) {
		vec2 ndc = (ClipSpaceCoords.xy/ClipSpaceCoords.w)/2.0 + 0.5;
		//minimum separation is in linear world space
		//first linearize z from prev layer depth buffer
		float prevLayerZ = texture(bufferDepthCompare, vec3(ndc, 0)).r;
		float prevLayerZLinear = 2.0 * prevLayerZ - 1.0;
		prevLayerZLinear = 2.0 * nearPlane * farPlane / (farPlane + nearPlane - prevLayerZLinear * (farPlane - nearPlane));
		//add minimum separation
		prevLayerZLinear += MINIMUM_SEPARATION;
		//convert back to nonlinear depth space
		float compareDepth = (farPlane + nearPlane - 2.0 * nearPlane * farPlane / prevLayerZLinear) / (farPlane - nearPlane);
		compareDepth = (compareDepth + 1.0) / 2.0;

		if(gl_FragCoord.z > compareDepth) {
			positionBuffer = FragPos;
			normalBuffer = normalize(Normal);
			colorBuffer.rgb = texture(texture_diffuse1, TexCoords).rgb;
			colorBuffer.a = texture(texture_specular1, TexCoords).r;
		}
		else
			discard;
	}
}  