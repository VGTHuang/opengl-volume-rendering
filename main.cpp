#include <main.h>

int imageX, imageY, imageZ;

class Timer
{
private:
	// Type aliases to make accessing nested type easier
	using clock_t = std::chrono::high_resolution_clock;
	using second_t = std::chrono::duration<double, std::ratio<1> >;

	std::chrono::time_point<clock_t> m_beg;

public:
	Timer() : m_beg(clock_t::now())
	{
	}

	void reset()
	{
		m_beg = clock_t::now();
	}

	double elapsed() const
	{
		return std::chrono::duration_cast<second_t>(clock_t::now() - m_beg).count();
	}
};

Timer timer;

void printTimer(const char *info) {
	double a = timer.elapsed();
	timer.reset();
	printf("%s %f\n", info, a);
}

void getImage3DConfig(std::string &image3dPath, std::string &transferPath, int &x, int &y, int &z) {
	std::string data;
	std::ifstream rfile;

	rfile.open("file_config.txt", std::ios::in | std::ios::out);

	rfile >> image3dPath;
	rfile >> transferPath;
	rfile >> x >> y >> z;

	rfile.close();

}

GLuint image3DTexObj, histogramTexObj, renderTexObj, transferTexObj;
bool firstRender = true;
glm::vec4 bg(0.2, 0.2, 0.2, 1.0);

void genTexImage3D(unsigned short *imgVals, glm::ivec3 img3DShape) {
	glGenTextures(1, &image3DTexObj);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, image3DTexObj);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R16, img3DShape.y, img3DShape.z, img3DShape.x, 0, GL_RED, GL_UNSIGNED_SHORT,
		imgVals);
}

void genHistogram(unsigned short *imgVals, glm::ivec3 img3DShape) {
	glBindImageTexture(0, image3DTexObj, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16);

	// output
	int out_tex_w = 512, out_tex_h = 512;

	glGenTextures(1, &histogramTexObj);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, histogramTexObj);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, out_tex_w, out_tex_h, 0, GL_RGBA, GL_FLOAT,
		NULL);
	glBindImageTexture(1, histogramTexObj, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);


	histogramComputeShader->use();
	glDispatchCompute(img3DShape.y, img3DShape.z, img3DShape.x);

	// glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// read output texture to cpu
	float *new_array = (float *)malloc(sizeof(float) * 4 * out_tex_w * out_tex_h);
	unsigned char *new_array_unsigned_char = (unsigned char *)malloc(sizeof(unsigned char) * 4 * out_tex_w * out_tex_h);

	glBindTexture(GL_TEXTURE_2D, histogramTexObj);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, new_array);
	
	for (int i = 0; i < out_tex_w * out_tex_h; i++) {
		int ii = out_tex_w * (out_tex_h - (i / out_tex_h) - 1)  + i % out_tex_h;
		new_array_unsigned_char[4 * i] = (unsigned char)(new_array[4 * ii] * 255);
		new_array_unsigned_char[4 * i + 1] = (unsigned char)(new_array[4 * ii + 1] * 255);
		new_array_unsigned_char[4 * i + 2] = (unsigned char)(new_array[4 * ii + 2] * 255);
		new_array_unsigned_char[4 * i + 3] = 255;
	}

	// save array to png

	CString histogramSavePath("");
	OpenWindowsDlg(false, true, true, 0, &histogramSavePath);
	stbi_write_png(histogramSavePath + "\\histogram.png", out_tex_w, out_tex_h, 4, new_array_unsigned_char, 0);

	free(new_array);
	free(new_array_unsigned_char);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void genTransfer(const std::string transferTexPath) {
	glDeleteTextures(1, &transferTexObj);
	transferTexObj = TextureLoader::loadTexture(transferTexPath.c_str());
}

void genRenderTex() {
	// output
	int out_tex_w = 512, out_tex_h = 512;
	// TODO too many generated textures!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	glGenTextures(1, &renderTexObj);
	glBindTexture(GL_TEXTURE_2D, renderTexObj);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, out_tex_w, out_tex_h, 0, GL_RGBA, GL_FLOAT,
		NULL);
}

void renderToTex() {
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glBindImageTexture(1, renderTexObj, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	clearComputeShader->use();
	glDispatchCompute(512, 512, 1);

	
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glBindImageTexture(1, image3DTexObj, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16);
	glBindImageTexture(2, renderTexObj, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, transferTexObj);

	renderComputeShader->use();
	glDispatchCompute(512, 512, 1);
	

	// glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glBindTexture(GL_TEXTURE_2D, 0);
}


// int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
int main()
{
	// config
	std::string image3DPath, transferPath;
	getImage3DConfig(image3DPath, transferPath, imageX, imageY, imageZ);

	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	std::string glsl_version = "";
#ifdef __APPLE__
	// GL 3.2 + GLSL 150
	glsl_version = "#version 150";
	glfwWindowHint( // required on Mac OS
		GLFW_OPENGL_FORWARD_COMPAT,
		GL_TRUE
	);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#elif __linux__
	// GL 3.2 + GLSL 150
	glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#elif _WIN32
	// GL 3.0 + GLSL 130 (???)
	glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "volume rendering", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// camera
	glm::vec3 camPos(-3.0f, -1.5f, -1.5f);
	camera = new Camera(camPos, SCR_WIDTH, SCR_HEIGHT, 2.0f, 0.5f);

	// glfw events
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	std::cout << glGetString(GL_VERSION) << std::endl;

	stbi_set_flip_vertically_on_load(true);

	// ******************************* read medical data *******************************

	glm::ivec3 imgShape(imageX, imageY, imageZ);
	FILE *fpsrc = NULL;
	unsigned short *imgValsUINT;
	imgValsUINT = (unsigned short *)malloc(sizeof(unsigned short) * imageX * imageY * imageZ);
	errno_t err = fopen_s(&fpsrc, image3DPath.c_str(), "r");
	if (err != 0)
	{
		printf("can not open the raw image");
		return 0;
	}
	else
	{
		printf("IMAGE read OK\n");
	}
	fread(imgValsUINT, sizeof(unsigned short), imageX * imageY * imageZ, fpsrc);
	fclose(fpsrc);

	unsigned short maxImgValue = 0;

	for (int i = 0; i < imageX * imageY * imageZ; i++) {
		if (imgValsUINT[i] > maxImgValue) {
			maxImgValue = imgValsUINT[i];
		}
	}


	// glEnable(GL_TEXTURE_3D);

	// unsigned int img3DTex = loadTexture3D(imgValsFLOAT, 1, glm::ivec3(imageZ, imageX, imageY));

	// shader
	drawShader = new Shader("VolVertexShader.glsl", "VolFragmentShader.glsl");
	histogramComputeShader = new Shader("HistogramComputeShaderNew.glsl");
	renderComputeShader = new Shader("RenderComputeShader.glsl");
	clearComputeShader = new Shader("ClearComputeShader.glsl");
	histogramComputeShader->use();
	histogramComputeShader->setInt("maxImgValue", maxImgValue);
	renderComputeShader->use();
	renderComputeShader->setInt("maxImgValue", maxImgValue);
	clearComputeShader->use();
	clearComputeShader->setVec4("bg", bg);

	
	// generate stuff
	genTexImage3D(imgValsUINT, imgShape);
	genTransfer(transferPath);
	genRenderTex();
	free(imgValsUINT);
	// ******************************* test *******************************

	float *cube = TextureLoader().getCube();

	unsigned int VAO, VBO;

	// total size of the buffer in bytes
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, 36 * 8 * sizeof(float), cube, GL_STATIC_DRAW);
	

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// tex attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);


	unsigned int uboMatrices;
	glGenBuffers(1, &uboMatrices);

	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 3 * sizeof(glm::mat4));

	glm::mat4 modelMat = glm::mat4(1.0f);

	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(modelMat));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);


	// ******************************* end test *******************************

	// imgui
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version.c_str());

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// imgui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			ImGui::Begin("controller");
			ImGui::Text("Use AWSD to control pitch/yaw; drag to pan camera");
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			if (ImGui::Button("download histogram")) {
				genHistogram(imgValsUINT, imgShape);
			}
			if (ImGui::Button("change transfer tex")) {
				CString transferLoadPath("");
				OpenWindowsDlg(false, true, false, 3, &transferLoadPath);
				std::cout << transferLoadPath << std::endl;
				genTransfer(std::string(transferLoadPath));
			}
			ImGui::ColorEdit4("background", (float*)&bg);
			ImGui::End();
		}

		// Rendering
		ImGui::Render();

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		

		glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		clearComputeShader->use();
		clearComputeShader->setVec4("bg", bg);

		renderToTex();
		glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(camera->GetViewMat4()));
		glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(camera->GetProjectionMat4()));
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		// bind textures on corresponding texture units
		// glActiveTexture(GL_TEXTURE0);
		// glBindTexture(GL_TEXTURE_3D, img3DTex);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, renderTexObj);

		drawShader->use();
		drawShader->setVec3("camPos", camera->GetCameraPos());
		// render boxes
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// render imgui
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}


	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();

	//system("pause");
	return 0;
}