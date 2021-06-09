#pragma once
#ifndef MAIN
#define MAIN

#define NOMINMAX
#include <Windows.h>
#include <Shlobj.h>
#include <atlstr.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shader_s.h"
#include <stb_image.h>
#include <stb_image_write.h>
#include <hhx_camera_1.0.h>
#include <texture_loader.h>

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
#include <algorithm>
#include <chrono>

// settings
const unsigned int SCR_WIDTH = 640;
const unsigned int SCR_HEIGHT = 640;

Shader *drawShader, *histogramComputeShader, *renderComputeShader, *clearComputeShader, *depthStencilShader;

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

unsigned int loadTexture2D(float const *data, const int nrComponents, const glm::ivec2 size, GLenum clamp = GL_REPEAT)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	unsigned int textureID;
	glGenTextures(1, &textureID);

	GLenum format;
	if (nrComponents == 1)
		format = GL_RED;
	else if (nrComponents == 3)
		format = GL_RGB;
	else if (nrComponents == 4)
		format = GL_RGBA;

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, format, GL_FLOAT, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // disables mipmap level!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return textureID;
}

unsigned int loadTexture3D(float const *data, const int nrComponents, const glm::ivec3 size, GLenum clamp = GL_REPEAT)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	unsigned int textureID;
	glGenTextures(1, &textureID);

	GLenum format;
	if (nrComponents == 1)
		format = GL_RED;
	else if (nrComponents == 3)
		format = GL_RGB;
	else if (nrComponents == 4)
		format = GL_RGBA;

	glBindTexture(GL_TEXTURE_3D, textureID);
	glTexImage3D(GL_TEXTURE_3D, 0, format, size.x, size.y, size.z, 0, format, GL_FLOAT, data);
	glGenerateMipmap(GL_TEXTURE_3D);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, clamp);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, clamp);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, clamp);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // disables mipmap level
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	return textureID;
}

std::string openfilename(const char *filter = "All Files (*.*)\0*.*\0", HWND owner = NULL) {
	
	OPENFILENAME ofn;
	char fileName[MAX_PATH] = "";
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "";

	std::string fileNameStr;
	
	if (GetOpenFileName(&ofn))
		fileNameStr = fileName;

	return fileNameStr;
}

// https://blog.csdn.net/Murphy_CoolCoder/article/details/89380888
//Windows API Open dialogbox.
//nType is a sign which deceide the dialog box filter format.
bool OpenWindowsDlg(bool isMultiSelect, bool IsOpen, bool IsPickFolder, int nType, CString *pFilePath)
{
	CoInitialize(nullptr);
	if (!isMultiSelect)
	{
		IFileDialog *pfd = NULL;
		HRESULT hr = NULL;
		if (IsOpen)
			hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
		else
			hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
		if (SUCCEEDED(hr))
		{
			DWORD dwFlags;
			hr = pfd->GetOptions(&dwFlags);
			if (IsPickFolder)
				hr = pfd->SetOptions(dwFlags | FOS_PICKFOLDERS);
			else
				hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
			switch (nType)
			{
			case 0:
			{
				COMDLG_FILTERSPEC fileType[] =
				{
					{ L"All files", L"*.*" },
					{ L"Text files",   L"*.txt*" },
					{ L"Pictures", L"*.png" },
				};
				hr = pfd->SetFileTypes(ARRAYSIZE(fileType), fileType);
				break;
			}
			case 1: //open or save recipe only allow file with extension .7z
			{
				COMDLG_FILTERSPEC fileType[] =
				{
					{ L"Reciep",L"*.7z*" },
				};
				hr = pfd->SetFileTypes(ARRAYSIZE(fileType), fileType);
				break;
			}
			case 2://Load or export file with different file extension
			{
				COMDLG_FILTERSPEC fileType[] =
				{
					{ L"Text",L"*.txt" },
					{ L"CSV",L".csv" },
					{ L"ini",L".ini" },
				};
				hr = pfd->SetFileTypes(ARRAYSIZE(fileType), fileType);
				break;
			}
			case 3: //Save as a print screen capture as  .png  format.
			{
				COMDLG_FILTERSPEC fileType[] =
				{
					{ L"Picture",L"*.png" },
				};
				hr = pfd->SetFileTypes(ARRAYSIZE(fileType), fileType);
				break;
			}
			case 4:
			{
				COMDLG_FILTERSPEC fileType[] =
				{
					{ L"Xml Document",L"*.xml*" },
				};
				hr = pfd->SetFileTypes(ARRAYSIZE(fileType), fileType);
				break;
			}
			default:
				break;
			}

			if (!IsOpen) //Save mode get file extension
			{
				if (nType == 1)
					hr = pfd->SetDefaultExtension(L"7z");
				if (nType == 3)
					hr = pfd->SetDefaultExtension(L"png");
				if (nType == 4)
					hr = pfd->SetDefaultExtension(L"xml");
			}
			hr = pfd->Show(NULL); //Show dialog
			if (SUCCEEDED(hr))
			{
				if (!IsOpen)       //Capture user change when select differen file extension.
				{
					if (nType == 2)
					{
						UINT  unFileIndex(1);
						hr = pfd->GetFileTypeIndex(&unFileIndex);
						switch (unFileIndex)
						{
						case 0:
							hr = pfd->SetDefaultExtension(L"txt");
							break;
						case 1:
							hr = pfd->SetDefaultExtension(L"csv");
							break;
						case 2:
							hr = pfd->SetDefaultExtension(L"ini");
							break;
						default:
							hr = pfd->SetDefaultExtension(L"txt");
							break;
						}
					}
				}
			}
			if (SUCCEEDED(hr))
			{
				IShellItem *pSelItem;
				hr = pfd->GetResult(&pSelItem);
				if (SUCCEEDED(hr))
				{
					LPWSTR pszFilePath = NULL;
					hr = pSelItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszFilePath);
					*pFilePath = pszFilePath;
					CoTaskMemFree(pszFilePath);
				}
				pSelItem->Release();
			}
		}
		pfd->Release();
	}
	else  //Open dialog with multi select allowed;
	{
		IFileOpenDialog *pfd = NULL;
		HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
		if (SUCCEEDED(hr))
		{
			DWORD dwFlags;
			hr = pfd->GetOptions(&dwFlags);
			hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT);
			COMDLG_FILTERSPEC fileType[] =
			{
				{ L"All files", L"*.*" },
				{ L"Text files",   L"*.txt*" },
				{ L"Pictures", L"*.png" },
			};
			hr = pfd->SetFileTypes(ARRAYSIZE(fileType), fileType);
			hr = pfd->SetFileTypeIndex(1);
			hr = pfd->Show(NULL);
			if (SUCCEEDED(hr))
			{
				IShellItemArray  *pSelResultArray;
				hr = pfd->GetResults(&pSelResultArray);
				if (SUCCEEDED(hr))
				{
					DWORD dwNumItems = 0; // number of items in multiple selection
					hr = pSelResultArray->GetCount(&dwNumItems);  // get number of selected items
					for (DWORD i = 0; i < dwNumItems; i++)
					{
						IShellItem *pSelOneItem = NULL;
						PWSTR pszFilePath = NULL; // hold file paths of selected items
						hr = pSelResultArray->GetItemAt(i, &pSelOneItem); // get a selected item from the IShellItemArray
						if (SUCCEEDED(hr))
						{
							hr = pSelOneItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
							if (SUCCEEDED(hr))
							{
								/*szSelected += pszFilePath;
								if (i < (dwNumItems - 1))
									szSelected += L"\n";*/
								CoTaskMemFree(pszFilePath);
							}
							pSelOneItem->Release();
						}
					}
					pSelResultArray->Release();
				}
			}
		}
		pfd->Release();
	}
	return true;
}
#endif