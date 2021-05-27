#pragma once
#ifndef MAIN
#define MAIN

#define NOMINMAX
#include <Windows.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shader_s.h"
#include <hhx_camera_1.0.h>

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include<algorithm>
#include <chrono>

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Shader *computeShader, *drawShader, *drawWireframeShader;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool lbutton_down = false;
double mouseLastX, mouseLastY;
Camera *camera;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	camera->ScrollCallback(yoffset);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	camera->MouseDragMoveCallback(lbutton_down, -xpos, -ypos);
}

// https://stackoverflow.com/questions/37194845/using-glfw-to-capture-mouse-dragging-c
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (GLFW_PRESS == action)
			lbutton_down = true;
		else if (GLFW_RELEASE == action)
			lbutton_down = false;
	}
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	camera->KeyDownCallback(window, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	camera->ResizeCallback(width, height);
}

void createSSBO(GLuint &newSSBO, const int memSize, const int bindingIndex, void *buffer, Shader *shader, const char *storageBlockName)
{
	glDeleteBuffers(1, &newSSBO);
	glGenBuffers(1, &newSSBO);
	int a = glGetError();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, newSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, memSize, buffer, GL_DYNAMIC_DRAW);
	// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, newSSBO);
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	shader->use();

	int block_index = glGetProgramResourceIndex(shader->ID, GL_SHADER_STORAGE_BLOCK, storageBlockName);
	glShaderStorageBlockBinding(shader->ID, block_index, bindingIndex);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingIndex, newSSBO);

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

#endif