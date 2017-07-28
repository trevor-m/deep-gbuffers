#pragma once
#include "glitter.hpp"
#include "shader.hpp"
#include "gbuffer.hpp"
#include <iostream>
#include <vector>
using std::vector;

#define RADIOSITY_NUM_SAMPLES 32

//Single-scatter radiosity
class RadiosityBuffer {
private:
	//frame buffer
	GLuint FBO;
	//inputs
	GLuint bufferRadiosity;
	GLuint bufferColor;
	//outputs
	//GLuint bufferBounceRadiosityOut;

	//samples
	vector<glm::vec3> samples;
	GLuint noiseTexture;
public:
	RadiosityBuffer() {
		//create samples
		for (GLuint i = 0; i < RADIOSITY_NUM_SAMPLES; i++) {
			//direction
			//x & y go from -1 to 1
			float x = (rand() / (float)RAND_MAX) * 2.0 - 1.0;
			float y = (rand() / (float)RAND_MAX) * 2.0 - 1.0;
			//z goes from 0 to 1 (hemisphere)
			float z = (rand() / (float)RAND_MAX);

			//normalize to clamp to edge of hemisphere
			glm::vec3 sample = glm::normalize(glm::vec3(x, y, z));
			//scale from 0 to 1(magnitude)
			sample *= (rand() / (float)RAND_MAX);

			//scale to be more distrubuted around the origin
			float scale = i / (float)RADIOSITY_NUM_SAMPLES;
			scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
			sample *= scale;
			samples.push_back(sample);
		}

		//create noise texture
		vector<glm::vec3> noise;
		for (GLuint i = 0; i < 16; i++) {
			glm::vec3 n((rand() / (float)RAND_MAX) * 2.0 - 1.0, (rand() / (float)RAND_MAX) * 2.0 - 1.0, 0.0f);
			noise.push_back(n);
		}
		//put into 4x4 texture
		glGenTextures(1, &noiseTexture);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &noise[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//create FBO
		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		//1st output, Lambertian(diffuse) goes into radiosity algorithm
		glGenTextures(1, &bufferRadiosity);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferRadiosity);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, mWidth, mHeight, GBUFFER_LAYERS, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, bufferRadiosity, 0);
		//2nd output, ambient + specular, gets added back later
		glGenTextures(1, &bufferColor);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferColor);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, mWidth, mHeight, GBUFFER_LAYERS, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, bufferColor, 0);

		GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, attachments);

		//make sure framebuffer was built successfully
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Error setting up Radiosity Buffer " << glGetError() << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void BindFramebuffer() {
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	}

	void BindBuffersRadiosity(Shader& radiosityShader, GBuffer& gbuffer) {
		gbuffer.BindBuffersRadiosity(radiosityShader);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		glUniform1i(glGetUniformLocation(radiosityShader.Program, "texNoise"), 2);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferRadiosity);
		glUniform1i(glGetUniformLocation(radiosityShader.Program, "bufferRadiosity"), 3);
		//glActiveTexture(GL_TEXTURE4);
		//glBindTexture(GL_TEXTURE_2D_ARRAY, bufferColor);
		//glUniform1i(glGetUniformLocation(radiosityShader.Program, "bufferColor"), 4);
	}

	void BindBuffersBlur(Shader& blurRadiosityShader) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferColor);
		glUniform1i(glGetUniformLocation(blurRadiosityShader.Program, "bufferColor"), 1);
	}

	void SetUniforms(Shader& ssaoShader) {
		//samples
		for (GLuint i = 0; i < RADIOSITY_NUM_SAMPLES; i++)
			glUniform3fv(glGetUniformLocation(ssaoShader.Program, ("samples[" + std::to_string(i) + "]").c_str()), 1, &samples[i][0]);
	}

	/*void CopyOutputsToInputs() {

	}*/

	/*void BindBuffersLighting(Shader& lightingShader, GBuffer& gbuffer) {
		//bind necessary textures from gbuffer
		gbuffer.BindBuffersLighting(lightingShader);
		//as well as output from SSAO shader
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, bufferBlurColor);
		glUniform1i(glGetUniformLocation(lightingShader.Program, "bufferOcclusion"), 6);
	}*/
};