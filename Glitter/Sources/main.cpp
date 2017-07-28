// Local Headers
#include "glitter.hpp"
// System Headers
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Standard Headers
#include <cstdio>
#include <cstdlib>
#include <ctime>

//// Helper functions
#include <camera.hpp>
#include <shader.hpp>
#include <model.hpp>
#include <filesystem.hpp>

// My Additions
#include "light.hpp"
#include "gbuffer.hpp"
#include "ambientocclusionbuffer.hpp"
#include "blurbuffer.hpp"
#include "environmentmap.hpp"
#include "radiositybuffer.hpp"


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

void RenderCube();
void RenderQuad();

Camera camera(glm::vec3(0.f, 0.f, 2.f));

// Toggle display of SSAO buffer
#define DISPLAY_SSAO 1 //scene with SSAO
#define DISPLAY_SSAO_BUFFER 2 //the SSAO occlusion buffer
int displayMode = 1;
int useRadiosity = 0; //turns radiosity on/off
int whichRad = 0; //radiosity from both, first layer only, second layer only

#define NUM_LIGHTS 3
#define MOVE_LIGHT_SPEED 0.5f
Light lights[NUM_LIGHTS];
int moveLight = 0; // Which light to control

int main(int argc, char * argv[]) {
	srand(time(0));
    // Load GLFW and Create a Window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    auto mWindow = glfwCreateWindow(mWidth, mHeight, "OpenGL", nullptr, nullptr);

    // Check for Valid Context
    if (mWindow == nullptr) {
        fprintf(stderr, "Failed to Create OpenGL Context");
        return EXIT_FAILURE;
    }

    // Create Context and Load OpenGL Functions
    glfwMakeContextCurrent(mWindow);
    gladLoadGL();
    fprintf(stderr, "OpenGL %s\n", glGetString(GL_VERSION));

	// Set callback functions
	glfwSetKeyCallback(mWindow, key_callback);
	glfwSetCursorPosCallback(mWindow, mouse_callback);
	glfwSetScrollCallback(mWindow, scroll_callback);
	glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glEnable(GL_DEPTH_TEST);

	// Create G-Buffer for deferred shading
	GBuffer gbuffer;
	// Create buffers and textures for SSAO and SSDO
	AmbientOcclusionBuffer ssao;
	// Buffer and textures to blur SSAO buffer before using it in the lighting pass
	BlurBuffer blur;
	// Create buffers for Radiosity
	RadiosityBuffer radiosity;

	// Shader for rendering to shadow depth maps (first 3 passes)
	Shader depthShader(FileSystem::getPath("Shaders/depthMap.vert.glsl").c_str(), FileSystem::getPath("Shaders/depthMap.frag.glsl").c_str(), FileSystem::getPath("Shaders/depthMap.geom.glsl").c_str());
	// Shader for first pass to gbuffer
	Shader geometryShader(FileSystem::getPath("Shaders/geometry.vert.glsl").c_str(), FileSystem::getPath("Shaders/geometry.frag.glsl").c_str(), FileSystem::getPath("Shaders/geometry.geom.glsl").c_str());
	// Shader for second pass (SSAO)
	Shader ssaoShader(FileSystem::getPath("Shaders/ssao.vert.glsl").c_str(), FileSystem::getPath("Shaders/ssao.frag.glsl").c_str());
	// Shader for third pass (blurring SSAO)
	Shader blurShader(FileSystem::getPath("Shaders/ssao.vert.glsl").c_str(), FileSystem::getPath("Shaders/blur.frag.glsl").c_str());
	// Shader for fourth pass (lighting)
	Shader lightingShader(FileSystem::getPath("Shaders/lighting.vert.glsl").c_str(), FileSystem::getPath("Shaders/lighting.frag.glsl").c_str(), FileSystem::getPath("Shaders/lighting.geom.glsl").c_str());
	// Shader for fifth pass (radiosity)
	Shader radiosityShader(FileSystem::getPath("Shaders/ssao.vert.glsl").c_str(), FileSystem::getPath("Shaders/radiosity.frag.glsl").c_str());
	Shader blurRadiosityShader(FileSystem::getPath("Shaders/ssao.vert.glsl").c_str(), FileSystem::getPath("Shaders/combine.frag.glsl").c_str());
	// Shader for fifth pass (rendering light sources as white cubes)
	Shader lightSourceShader(FileSystem::getPath("Shaders/geometry.vert.glsl").c_str(), FileSystem::getPath("Shaders/lightSource.frag.glsl").c_str());
	// Shader for sixth pass (rendering environment map as a cube at infinity)
	Shader envShader(FileSystem::getPath("Shaders/envMap.vert.glsl").c_str(), FileSystem::getPath("Shaders/envMap.frag.glsl").c_str());

	// Load a model from obj file
	Model sampleModel(FileSystem::getPath("Resources/crytek_sponza/sponza.obj").c_str());

	// Load environment map
	std::vector<std::string> mapFiles;
	mapFiles.push_back(FileSystem::getPath("Resources/environment/Yokohama2/posx.jpg"));
	mapFiles.push_back(FileSystem::getPath("Resources/environment/Yokohama2/negx.jpg"));
	mapFiles.push_back(FileSystem::getPath("Resources/environment/Yokohama2/posy.jpg"));
	mapFiles.push_back(FileSystem::getPath("Resources/environment/Yokohama2/negy.jpg"));
	mapFiles.push_back(FileSystem::getPath("Resources/environment/Yokohama2/posz.jpg"));
	mapFiles.push_back(FileSystem::getPath("Resources/environment/Yokohama2/negz.jpg"));
	EnvironmentMap envMap(mapFiles);

	// Create lights
	lights[0] = Light(glm::vec3(0, 5, 0), glm::vec3(1, 1, 1), 0.0019, 0.022, 1.0, 0);
	lights[1] = Light(glm::vec3(52, 5, 10), glm::vec3(1, 1, 1), 0.0019, 0.022, 1.0, 1);
	lights[2] = Light(glm::vec3(-25, 35, -8), glm::vec3(1, 1, 1), 0.0007, 0.0014, 1.0, 2);

	// Background Fill Color
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Rendering Loop
    while (glfwWindowShouldClose(mWindow) == false) {
		glfwPollEvents();

		//mvp matrices
		glm::mat4 model;
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (GLfloat)mWidth / (GLfloat)mHeight, mNear, mFar);

		//render to each light's depth cubemap
		depthShader.Use();
		for (int i = 0; i < NUM_LIGHTS; i++) {
			lights[i].BindFramebuffer(depthShader, view);
			model = glm::mat4();
			model = glm::scale(model, glm::vec3(0.05f));    // The sponza model is too big, scale it first
			glUniformMatrix4fv(glGetUniformLocation(depthShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
			glUniformMatrix4fv(glGetUniformLocation(depthShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
			sampleModel.Draw(depthShader);
		}

		//1st pass: render to gbuffer
		gbuffer.BindFramebuffer();
		gbuffer.CopyAndBindDepthCompareLayer(geometryShader);
		glViewport(0, 0, mWidth, mHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//render model
		geometryShader.Use();
		glUniform1f(glGetUniformLocation(geometryShader.Program, "farPlane"), mFar);
		glUniform1f(glGetUniformLocation(geometryShader.Program, "nearPlane"), mNear);
		glUniformMatrix4fv(glGetUniformLocation(geometryShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(geometryShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		model = glm::mat4();
		model = glm::scale(model, glm::vec3(0.05f));    // The sponza model is too big, scale it first
		glUniformMatrix4fv(glGetUniformLocation(geometryShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		sampleModel.Draw(geometryShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//2nd pass: create ssao and render to quad
		ssao.BindFramebuffer();
		glClear(GL_COLOR_BUFFER_BIT);
		ssaoShader.Use();
		ssao.BindBuffersSSAO(ssaoShader, gbuffer);
		ssao.SetUniforms(ssaoShader);
		//glUniform1i(glGetUniformLocation(ssaoShader.Program, "which"), whichSSAO);
		//glUniformMatrix4fv(glGetUniformLocation(ssaoShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(ssaoShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		RenderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//3rd pass: blur ssao/ssdo output
		blur.BindFramebuffer();
		glClear(GL_COLOR_BUFFER_BIT);
		blurShader.Use();
		ssao.BindBuffersBlur(blurShader);
		RenderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//4th pass: lighting
		if (useRadiosity) {
			radiosity.BindFramebuffer();
			glClear(GL_COLOR_BUFFER_BIT);
		}
		else
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		lightingShader.Use();
		blur.BindBuffersLighting(lightingShader, gbuffer);
		//set up lighting uniforms including shadow depth cubemaps
		for (int i = 0; i < NUM_LIGHTS; i++) {
			lights[i].SetUniforms(lightingShader, view);
			lights[i].BindBuffers(lightingShader);
		}
		glUniform1f(glGetUniformLocation(lightingShader.Program, "farPlane"), mFar);
		glUniform1i(glGetUniformLocation(lightingShader.Program, "displayMode"), displayMode);
		RenderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//5th pass: radiosity
		if (useRadiosity) {
			//glBindFramebuffer(GL_FRAMEBUFFER, 0);
			blur.BindFramebuffer();
			glClear(GL_COLOR_BUFFER_BIT);
			radiosityShader.Use();
			radiosity.BindBuffersRadiosity(radiosityShader, gbuffer);
			radiosity.SetUniforms(radiosityShader);
			glUniform1i(glGetUniformLocation(radiosityShader.Program, "which"), whichRad);
			glUniformMatrix4fv(glGetUniformLocation(radiosityShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			RenderQuad();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			//6th: blur radiosity, combine with rest of lighting and display
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			blurRadiosityShader.Use();
			blur.BindBuffersRadiosity(blurRadiosityShader);
			radiosity.BindBuffersBlur(blurRadiosityShader);
			RenderQuad();
		}

		//7th: render a cube for each light source
		//copy depth buffer from gbuffer to properly occlude light sources
		glClear(GL_DEPTH_BUFFER_BIT);
		gbuffer.CopyDepthBuffer();
		lightSourceShader.Use();
		glUniformMatrix4fv(glGetUniformLocation(lightSourceShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(lightSourceShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		for (int i = 0; i < NUM_LIGHTS; i++) {
			model = glm::mat4();
			model = glm::translate(model, lights[i].position);
			glUniformMatrix4fv(glGetUniformLocation(lightSourceShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(glGetUniformLocation(lightSourceShader.Program, "lightColor"), 1, &lights[i].specular[0]);
			RenderCube();
		}

		//finally: render environment map wherever depth = infinity
		glDepthFunc(GL_LEQUAL);
		envShader.Use();
		model = glm::mat4();
		model = glm::scale(model, glm::vec3(2.0f));
		//ignore translation component of view matrix
		view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
		glUniformMatrix4fv(glGetUniformLocation(envShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(glGetUniformLocation(envShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(envShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		envMap.BindBuffers(envShader);
		RenderCube();
		glDepthFunc(GL_LESS);

        // Flip Buffers and Draw
        glfwSwapBuffers(mWindow);
    }
	glfwTerminate();
    return EXIT_SUCCESS;
}

// RenderQuad() Renders a quad that fills the screen
GLuint quadVAO = 0;
GLuint quadVBO;
void RenderQuad()
{
	if (quadVAO == 0)
	{
		GLfloat quadVertices[] = {
			// Positions        // Texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// Setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

// RenderCube() Renders a 1x1 3D cube in NDC.
GLuint cubeVAO = 0;
GLuint cubeVBO = 0;
void RenderCube()
{
	// Initialize (if necessary)
	if (cubeVAO == 0)
	{
		GLfloat vertices[] = {
			// Back face
			-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // Bottom-left
			0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,  // top-right
			-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,  // bottom-left
			-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,// top-left
			// Front face
			-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,  // bottom-right
			0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,  // top-right
			0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,  // top-left
			-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,  // bottom-left
			// Left face
			-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,  // bottom-left
			-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,  // bottom-right
			-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// Right face
			0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,  // bottom-right
			0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,  // top-left
			0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// Bottom face
			-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,// bottom-left
			0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// Top face
			-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,// top-left
			0.5f,  0.5f , 0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,// top-left
			-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// Fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// Link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// Render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// Camera movements
	if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
		camera.ProcessKeyboard(FORWARD, 0.1);
	if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
		camera.ProcessKeyboard(BACKWARD, 0.1);
	if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
		camera.ProcessKeyboard(LEFT, 0.1);
	if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
		camera.ProcessKeyboard(RIGHT, 0.1);

	// Switch display mode
	if (key == GLFW_KEY_7 && action == GLFW_PRESS)
		displayMode = DISPLAY_SSAO;
	if (key == GLFW_KEY_8 && action == GLFW_PRESS)
		displayMode = DISPLAY_SSAO_BUFFER;

	if (key == GLFW_KEY_9 && action == GLFW_PRESS)
		whichRad = (whichRad + 1) % 3;

	// Toggle radiosity
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
		useRadiosity = !useRadiosity;

	// Set which light to move
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
		moveLight = 0;
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
		moveLight = 1;
	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
		moveLight = 2;
	
	// Light movement
	if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT))
		lights[moveLight].position.z += MOVE_LIGHT_SPEED;
	if (key == GLFW_KEY_K && (action == GLFW_PRESS || action == GLFW_REPEAT))
		lights[moveLight].position.z -= MOVE_LIGHT_SPEED;
	if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT))
		lights[moveLight].position.x += MOVE_LIGHT_SPEED;
	if (key == GLFW_KEY_L && (action == GLFW_PRESS || action == GLFW_REPEAT))
		lights[moveLight].position.x -= MOVE_LIGHT_SPEED;
	if (key == GLFW_KEY_U && (action == GLFW_PRESS || action == GLFW_REPEAT))
		lights[moveLight].position.y += MOVE_LIGHT_SPEED;
	if (key == GLFW_KEY_O && (action == GLFW_PRESS || action == GLFW_REPEAT))
		lights[moveLight].position.y -= MOVE_LIGHT_SPEED;
}

GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if (state == GLFW_PRESS) {
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		GLfloat xoffset = xpos - lastX;
		GLfloat yoffset = lastY - ypos;

		lastX = xpos;
		lastY = ypos;

		camera.ProcessMouseMovement(xoffset, yoffset);
	}
	if (state == GLFW_RELEASE) {
		firstMouse = true;
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}