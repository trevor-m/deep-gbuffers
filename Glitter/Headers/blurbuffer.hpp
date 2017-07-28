#pragma once
#include "glitter.hpp"
#include "shader.hpp"
#include "gbuffer.hpp"
#include <iostream>
#include <vector>
using std::vector;

//Single color blur
class BlurBuffer {
private:
	//frame buffer
	GLuint FBO;
	//output texture
	GLuint bufferBlurColor;
public:
	BlurBuffer() {
		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glGenTextures(1, &bufferBlurColor);
		glBindTexture(GL_TEXTURE_2D, bufferBlurColor);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferBlurColor, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void BindFramebuffer() {
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	}


	void BindBuffersLighting(Shader& lightingShader, GBuffer& gbuffer) {
		//bind necessary textures from gbuffer
		gbuffer.BindBuffersLighting(lightingShader);
		//as well as output from SSAO shader
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, bufferBlurColor);
		glUniform1i(glGetUniformLocation(lightingShader.Program, "bufferOcclusion"), 6);
	}

	void BindBuffersRadiosity(Shader& blurRadiosityShader) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, bufferBlurColor);
		glUniform1i(glGetUniformLocation(blurRadiosityShader.Program, "bufferRadiosity"), 0);
	}
};