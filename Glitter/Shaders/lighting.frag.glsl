#version 430 core

#define NUM_LIGHTS 3
struct PointLight {
	vec3 pos;
	float a;
	float b;
	float c;
	
	vec3 ambient;
	vec3 specular;
	vec3 diffuse;
};

layout (location = 1) out vec3 radiosity;
layout (location = 0) out vec4 color;
in vec2 TexCoords;

//GBuffer data
uniform sampler2DArray bufferPosition;
uniform sampler2DArray bufferNormal;
uniform sampler2DArray bufferColor;
//SSAO occlusion factor
uniform sampler2D bufferOcclusion;

//Lighting information
uniform samplerCube depthMap[NUM_LIGHTS];
uniform float farPlane;
uniform PointLight lights[NUM_LIGHTS];

//display SSAO buffer
//1 = v
//2 = SSAO buffer
uniform int displayMode;

vec3 applyPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor, float specularColor, float occlusion, int i, out vec3 diffuse)
{
	vec3 lightDir = normalize(light.pos - fragPos);

	//in shadow?
	vec3 fragToLight = fragPos - light.pos; 
    float closestDepth = texture(depthMap[i], fragToLight).r;
	closestDepth *= farPlane;
	float currentDepth = length(fragToLight);
    //apply a bias to avoid moire artifacts
    //float bias = 0.15; 
	//higher biased the further light dir is from normal
	float bias = max(0.15 * (1.0 - dot(normal, lightDir)), 0.5);  
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	//distance attenuation
	float dist = length(light.pos - fragPos);
	float atten = 1.0/(light.a*dist*dist + light.b*dist + light.c);

	//diffuse lighting
	float diff = max(dot(normal, lightDir), 0.0);
	diffuse = atten * diff * light.diffuse * diffuseColor;
	diffuse *= (1.0 - shadow);
	//second layer doesnt need color, just the diffuse componenet
	if(gl_Layer == 1)
		return vec3(0);

	//specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = atten * spec * light.specular * vec3(specularColor, specularColor, specularColor);
	specular *= (1.0 - shadow);

	//ambient
	vec3 ambient = atten * light.ambient * diffuseColor * occlusion.r;
	
	return ambient + diffuse + specular;
}

void main()
{
	vec3 FragPos = texture(bufferPosition, vec3(TexCoords, gl_Layer)).rgb;
    vec3 Normal = texture(bufferNormal, vec3(TexCoords, gl_Layer)).rgb;
    vec3 Diffuse = texture(bufferColor, vec3(TexCoords, gl_Layer)).rgb;
    float Specular = texture(bufferColor, vec3(TexCoords, gl_Layer)).a;
	float Occlusion = texture(bufferOcclusion, TexCoords).r;

	vec3 finalColor = vec3(0,0,0);
	vec3 finalRadiosity = vec3(0,0,0);

	//add each light
	vec3 viewDir = normalize(-FragPos);
	vec3 lambertian;
	for(int i = 0; i < NUM_LIGHTS; i++) {
		finalColor += applyPointLight(lights[i], Normal, FragPos, viewDir, Diffuse, Specular, Occlusion, i, lambertian);
		finalRadiosity += lambertian;
	}
	if(gl_Layer == 0) {
		if(displayMode == 1)
			color = vec4(finalColor, 1.0);
		else if(displayMode == 2)
			color = vec4(vec3(Occlusion), 1.0);
	}
	radiosity = finalRadiosity;
}