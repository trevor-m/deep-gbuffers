#pragma once
#include "glitter.hpp"
#include "shader.hpp"
#include <iostream>
#include <vector>

//2 depth layers
#define GBUFFER_LAYERS 2

//G-Buffer for deferred shading
class GBuffer {
private:
	//frame buffer object to render geometry to
	GLuint FBO;
	//buffer attachment textures where data is stored
	GLuint bufferPosition;
	GLuint bufferNormal;
	GLuint bufferColor;
	//GLuint bufferLambertian;
	//GLuint bufferSpecular;
	GLuint bufferDepth;

	//previous frame's first layer depth buffer
	GLuint bufferDepthCompare;

public:
	GBuffer() {
		//create framebuffer
		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);

		//create textures for each layer
		//position buffer - a "color" buffer
		glGenTextures(1, &bufferPosition);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferPosition);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB16F, mWidth, mHeight, GBUFFER_LAYERS, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, bufferPosition, 0);

		//normal buffer - a "color" buffer, use rgb for xyz
		glGenTextures(1, &bufferNormal);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferNormal);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB16F, mWidth, mHeight, GBUFFER_LAYERS, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, bufferNormal, 0);

		//color buffer - put diffuse in rgb, specular in a
		glGenTextures(1, &bufferColor);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferColor);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, mWidth, mHeight, GBUFFER_LAYERS, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, bufferColor, 0);

		//set as attachments to FBO
		GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, attachments);

		//add depth buffer
		//depth buffer has an extra layer, at bufferDepth[GBUFFER_LAYERS] which is used for comparison
		glGenTextures(1, &bufferDepth);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferDepth);
		//glTexStorage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, mWidth, mHeight, GBUFFER_LAYERS + 1);
		//glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, mWidth, mHeight, GBUFFER_LAYERS + 1, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, mWidth, mHeight, GBUFFER_LAYERS, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		//glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, bufferDepth, 0);

		//make sure framebuffer was built successfully
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Error setting up G-Buffer " << glGetError() << std::endl;

		//unbind FBO
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//create comparison depth buffer
		glGenTextures(1, &bufferDepthCompare);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferDepthCompare);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, mWidth, mHeight, GBUFFER_LAYERS - 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	void BindFramebuffer() {
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	}

	void CopyAndBindDepthCompareLayer(Shader& geometryShader) {
		//copy first layer of last frame's depth buffer into comparison texture and bind it
		glCopyImageSubData(bufferDepth, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, bufferDepthCompare, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, mWidth, mHeight, GBUFFER_LAYERS - 1);

		//and bind to texture unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferDepthCompare);
		glUniform1i(glGetUniformLocation(geometryShader.Program, "bufferDepthCompare"), 0);
	}

	void BindBuffersSSAO(Shader& ssaoShader) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferNormal);

		glUniform1i(glGetUniformLocation(ssaoShader.Program, "bufferPosition"), 0);
		glUniform1i(glGetUniformLocation(ssaoShader.Program, "bufferNormal"), 1);
	}

	void BindBuffersLighting(Shader& lightingShader) {
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferPosition);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferNormal);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferColor);

		glUniform1i(glGetUniformLocation(lightingShader.Program, "bufferPosition"), 3);
		glUniform1i(glGetUniformLocation(lightingShader.Program, "bufferNormal"), 4);
		glUniform1i(glGetUniformLocation(lightingShader.Program, "bufferColor"), 5);
	}

	void BindBuffersRadiosity(Shader& radiosityShader) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, bufferNormal);
		//glActiveTexture(GL_TEXTURE3);
		//glBindTexture(GL_TEXTURE_2D_ARRAY, bufferColor);

		glUniform1i(glGetUniformLocation(radiosityShader.Program, "bufferPosition"), 0);
		glUniform1i(glGetUniformLocation(radiosityShader.Program, "bufferNormal"), 1);
		//glUniform1i(glGetUniformLocation(radiosityShader.Program, "bufferRadiosity"), 3);
	}

	// Copies depth buffer from G-Buffer to standard framebuffer 0
	void CopyDepthBuffer() {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
};