#pragma once
#include "glitter.hpp"
#include "shader.hpp"
#include "gbuffer.hpp"
#include <iostream>
#include <vector>
using std::vector;

#define SSAO_NUM_SAMPLES 32

//SSAO
class AmbientOcclusionBuffer {
private:
	//for SSAO
	vector<glm::vec3> samples;
	GLuint noiseTexture;
	GLuint FBO;
	GLuint bufferSSAO;
public:
	AmbientOcclusionBuffer() {
		//create samples
		for (GLuint i = 0; i < SSAO_NUM_SAMPLES; i++) {
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
			float scale = i / (float)SSAO_NUM_SAMPLES;
			scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
			sample *= scale;
			samples.push_back(sample);
		}
	
		//create noise
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

		//create fbo to store results of ssao stage
		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glGenTextures(1, &bufferSSAO);
		glBindTexture(GL_TEXTURE_2D, bufferSSAO);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferSSAO, 0);
	}

	void BindFramebuffer() {
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	}

	void BindBuffersSSAO(Shader& ssaoShader, GBuffer& gbuffer) {
		//bind necessary textures from gbuffer
		gbuffer.BindBuffersSSAO(ssaoShader);
		//as well as noise texture
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		glUniform1i(glGetUniformLocation(ssaoShader.Program, "texNoise"), 2);
	}

	void BindBuffersBlur(Shader& blurShader) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, bufferSSAO);
		glUniform1i(glGetUniformLocation(blurShader.Program, "bufferInput"), 0);
	}

	void SetUniforms(Shader& ssaoShader) {
		//samples
		for (GLuint i = 0; i < SSAO_NUM_SAMPLES; i++)
			glUniform3fv(glGetUniformLocation(ssaoShader.Program, ("samples[" + std::to_string(i) + "]").c_str()), 1, &samples[i][0]);
	}
};