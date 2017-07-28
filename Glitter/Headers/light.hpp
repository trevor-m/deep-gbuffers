#pragma once

#include "glitter.hpp"
#include "shader.hpp"
#include <glm/glm.hpp>

#define SHADOW_MAP_RESOLUTION 1024
#define SHADOW_NEAR 1.0f
#define SHADOW_FAR 25.0f

class Light {
private:
	//Shadows
	//depth map
	GLuint depthCubemap;
	//framebuffer
	GLuint depthMapFBO;
	
	void initializeDepthMap() {
		//create shadow cubemap
		glGenTextures(1, &depthCubemap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
		for (GLuint i = 0; i < 6; i++)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		//create FBO
		glGenFramebuffers(1, &depthMapFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	//Unique ID
	int id;
public:
	//Light Properties
	//location
	glm::vec3 position;
	//colors
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	//attenuation constants
	float a, b, c;

	Light() {
		ambient = glm::vec3(0.1, 0.1, 0.1);
		diffuse = glm::vec3(0.7, 0.7, 0.7);
		specular = glm::vec3(1.0, 1.0, 1.0);
		a = 0.017;
		b = 0.07;
		c = 1.0;
	}

	Light(glm::vec3 position, glm::vec3 color, float a, float b, float c, int id) {
		this->position = position;
		this->ambient = color * 0.4f;
		this->diffuse = color * 0.7f;
		this->specular = color;
		this->a = a;
		this->b = b;
		this->c = c;
		this->id = id;

		initializeDepthMap();
	}

	Light(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float a, float b, float c, int id) {
		this->position = position;
		this->ambient = ambient;
		this->diffuse = diffuse;
		this->specular = specular;
		this->a = a;
		this->b = b;
		this->c = c;
		this->id = id;

		initializeDepthMap();
	}

	void BindFramebuffer(Shader& depthShader, glm::mat4 view) {
		glViewport(0, 0, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);

		//also set uniforms needed for depth map rendering
		//convert position to view space
		glm::vec3 posView = glm::vec3(view * glm::vec4(position, 1.0));

		//transformations to light-space for each face
		std::vector<glm::mat4> shadowTransforms;

		//create light-space transformation matrices for each cube face
		glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, SHADOW_NEAR, SHADOW_FAR);
		shadowTransforms.push_back(shadowProj * glm::lookAt(posView, posView + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(posView, posView + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(posView, posView + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(posView, posView + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(posView, posView + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(posView, posView + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

		//set uniforms
		depthShader.Use();
		for (GLuint i = 0; i < 6; ++i)
			glUniformMatrix4fv(glGetUniformLocation(depthShader.Program, ("shadowMatrices[" + std::to_string(i) + "]").c_str()), 1, GL_FALSE, glm::value_ptr(shadowTransforms[i]));
		glUniform1f(glGetUniformLocation(depthShader.Program, "farPlane"), mFar);
		glUniform3fv(glGetUniformLocation(depthShader.Program, "lightPos"), 1, &posView[0]);
	}

	void SetUniforms(Shader& lightShader, glm::mat4 view) {
		lightShader.Use();
		//light properties
		//convert position to view space
		glm::vec3 lightPosView = glm::vec3(view * glm::vec4(position, 1.0));
		glUniform3f(glGetUniformLocation(lightShader.Program, ("lights[" + to_string(id) + "].pos").c_str()), lightPosView.x, lightPosView.y, lightPosView.z);
		glUniform3f(glGetUniformLocation(lightShader.Program, ("lights[" + to_string(id) + "].ambient").c_str()), ambient.r, ambient.g, ambient.b);
		glUniform3f(glGetUniformLocation(lightShader.Program, ("lights[" + to_string(id) + "].diffuse").c_str()), diffuse.r, diffuse.g, diffuse.b);
		glUniform3f(glGetUniformLocation(lightShader.Program, ("lights[" + to_string(id) + "].specular").c_str()), specular.r, specular.g, specular.b);
		glUniform1f(glGetUniformLocation(lightShader.Program, ("lights[" + to_string(id) + "].a").c_str()), a);
		glUniform1f(glGetUniformLocation(lightShader.Program, ("lights[" + to_string(id) + "].b").c_str()), b);
		glUniform1f(glGetUniformLocation(lightShader.Program, ("lights[" + to_string(id) + "].c").c_str()), c);
	}

	void BindBuffers(Shader& lightShader) {
		//bind depthmap to texture unit #id
		glActiveTexture(GL_TEXTURE0 + id);
		glUniform1i(glGetUniformLocation(lightShader.Program, ("depthMap[" + to_string(id) + "]").c_str()), id);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
	}
};