#pragma once
#include "glitter.hpp"
#include <soil/soil.h>
#include <vector>
#include <string>

class EnvironmentMap {
private:
	//cubemap texture
	GLuint cubeMap;

public:
	//generate environment map
	//takes a vector of paths to each face in the following order:
	//right, left, top, bottom, back, front
	EnvironmentMap(std::vector<std::string> faceTextures) {
		//create cubemap
		glGenTextures(1, &cubeMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

		//load environment map texture for each face
		for (GLuint i = 0; i < 6; i++) {
			int width, height;
			unsigned char* image = NULL;
			image = SOIL_load_image(faceTextures[i].c_str(), &width, &height, 0, SOIL_LOAD_RGB);
			if (image == NULL) {
				std::cout << "EnvironmentMap could not load texture " << faceTextures[i] << std::endl;
			}
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		}

		//set cubemap settings
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	void BindBuffers(Shader& envShader) {
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(envShader.Program, "envMap"), 0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	}

	void BindBuffers(Shader& envShader, int textureUnit) {
		glActiveTexture(GL_TEXTURE0 + textureUnit);
		glUniform1i(glGetUniformLocation(envShader.Program, "envMap"), textureUnit);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	}
};