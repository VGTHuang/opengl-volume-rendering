#include <main.h>

const int canvasSize = 512;
const int transferSize = 512;
int imageX, imageY, imageZ;
GLuint image3DTexObj, histogramTexObj, renderTexObj, transferTexObj, framebufferObj, renderbufferObj, textureColorbufferObj;
int cubeVAO, planeVAO;
glm::vec4 bg(0.06, 0.06, 0.06, 1.0);
float opacity = 1.0;
int sampleCount = 30;

// get paths to image3D & transfer function; get length, width, height of image3D
void getImage3DConfig(std::string &image3dPath, std::string &transferPath, int &x, int &y, int &z) {
	std::string data;
	std::ifstream rfile;

	rfile.open("file_config.txt", std::ios::in | std::ios::out);

	rfile >> image3dPath;
	rfile >> transferPath;
	rfile >> x >> y >> z;

	rfile.close();
}


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

	glGenTextures(1, &histogramTexObj);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, histogramTexObj);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, transferSize, transferSize, 0, GL_RGBA, GL_FLOAT,
		NULL);
	glBindImageTexture(1, histogramTexObj, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);


	histogramComputeShader->use();
	glDispatchCompute(img3DShape.y, img3DShape.z, img3DShape.x);

	// glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// read output texture to cpu
	float *new_array = (float *)malloc(sizeof(float) * 4 * transferSize * transferSize);
	unsigned char *new_array_unsigned_char = (unsigned char *)malloc(sizeof(unsigned char) * 4 * transferSize * transferSize);

	glBindTexture(GL_TEXTURE_2D, histogramTexObj);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, new_array);
	
	for (int i = 0; i < transferSize * transferSize; i++) {
		int j = transferSize * (transferSize - (i / transferSize) - 1)  + i % transferSize;
		new_array_unsigned_char[4 * i] = (unsigned char)(new_array[4 * j] * 255);
		new_array_unsigned_char[4 * i + 1] = (unsigned char)(new_array[4 * j + 1] * 255);
		new_array_unsigned_char[4 * i + 2] = (unsigned char)(new_array[4 * j + 2] * 255);
		new_array_unsigned_char[4 * i + 3] = 255;
	}

	// save array to png

	CString histogramSavePath("");
	OpenWindowsDlg(false, true, true, 0, &histogramSavePath);
	stbi_write_png(histogramSavePath + "\\histogram.png", transferSize, transferSize, 4, new_array_unsigned_char, 0);

	free(new_array);
	free(new_array_unsigned_char);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void genTransfer(const std::string transferTexPath) {
	glDeleteTextures(1, &transferTexObj);
	transferTexObj = TextureLoader::loadTexture(transferTexPath.c_str(), GL_REPEAT);
}

void genRenderTex() {
	// output
	glGenTextures(1, &renderTexObj);
	glBindTexture(GL_TEXTURE_2D, renderTexObj);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, canvasSize, canvasSize, 0, GL_RGBA, GL_FLOAT,
		NULL);

	// https://stackoverflow.com/questions/25252512/how-can-i-pass-multiple-textures-to-a-single-shader
	// Get the uniform variables location. You've probably already done that before...
	int transferTexLocation = glGetUniformLocation(renderComputeShader->ID, "transfer");
	int depthTexLocation = glGetUniformLocation(renderComputeShader->ID, "depth");

	// Then bind the uniform samplers to texture units:
	renderComputeShader->use();
	glUniform1i(transferTexLocation, 0);
	glUniform1i(depthTexLocation, 1);
}

// https://learnopengl.com/Advanced-OpenGL/Framebuffers
void genDepthStencilFBO() {
	// framebuffer configuration
    // -------------------------
	glGenFramebuffers(1, &framebufferObj);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferObj);
	// create a color attachment texture
	glGenTextures(1, &textureColorbufferObj);
	glBindTexture(GL_TEXTURE_2D, textureColorbufferObj);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, canvasSize, canvasSize, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbufferObj, 0);
	// create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
	glGenRenderbuffers(1, &renderbufferObj);
	glBindRenderbuffer(GL_RENDERBUFFER, renderbufferObj);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, canvasSize, canvasSize); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbufferObj); // now actually attach it
	// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// https://learnopengl.com/Advanced-OpenGL/Framebuffers
void renderToDepth() {
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferObj);
	glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

	glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// make sure we clear the framebuffer's content
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	depthStencilShader->use();

	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindVertexArray(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderToTex() {
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glBindImageTexture(1, renderTexObj, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	clearComputeShader->use();
	clearComputeShader->setVec4("bg", bg);
	glDispatchCompute(canvasSize, canvasSize, 1);

	
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glBindImageTexture(1, image3DTexObj, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16);
	glBindImageTexture(2, renderTexObj, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	// glBindImageTexture(3, textureColorbufferObj, 0, FALSE, 0, GL_READ_ONLY, GL_R32F);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, transferTexObj);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textureColorbufferObj);
	int a = glGetError();
	renderComputeShader->use();
	renderComputeShader->setVec4("bg", bg);
	renderComputeShader->setFloat("opacity", opacity);
	renderComputeShader->setInt("sampleCount", sampleCount);
	glDispatchCompute(canvasSize / 4, canvasSize / 4, 1);
	

	// glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glBindTexture(GL_TEXTURE_2D, 0);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
// int main()
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
	GLFWwindow* window = glfwCreateWindow(canvasSize, canvasSize, "volume rendering", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// camera
	glm::vec3 camPos(3.0f, 2.0f, 2.0f);
	camera = new Camera(camPos, canvasSize, canvasSize, 1.0f, 0.5f);

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
	histogramComputeShader = new Shader("HistogramComputeShader.glsl");
	renderComputeShader = new Shader("RenderComputeShader.glsl");
	clearComputeShader = new Shader("ClearComputeShader.glsl");
	depthStencilShader = new Shader("DepthStencilVertexShader.glsl", "DepthStencilFragmentShader.glsl");

	histogramComputeShader->use();
	histogramComputeShader->setInt("maxImgValue", maxImgValue);
	renderComputeShader->use();
	renderComputeShader->setInt("maxImgValue", maxImgValue);
	renderComputeShader->setVec4("bg", bg);
	renderComputeShader->setFloat("canvasSize", (float)canvasSize);
	clearComputeShader->use();
	clearComputeShader->setVec4("bg", bg);

	
	// generate stuff
	genTexImage3D(imgValsUINT, imgShape);
	genTransfer(transferPath);
	genRenderTex();
	genDepthStencilFBO();
	free(imgValsUINT);

	// plane for rendering to screen plane
	// cube for rendering the volume's depth & stencil
	planeVAO = TextureLoader().getPlaneVAO(2.0);
	cubeVAO = TextureLoader().getCubeVAO();

	// uniform buffer for renderComputeShader & depthStencilShader; stores projection & view matrices
	unsigned int uboMatrices;
	glGenBuffers(1, &uboMatrices);

	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4) + sizeof(glm::vec3), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(glm::mat4));


	unsigned int renderMatrices = glGetUniformBlockIndex(renderComputeShader->ID, "Matrices");
	glUniformBlockBinding(renderComputeShader->ID, renderMatrices, 0);
	unsigned int depthMatrices = glGetUniformBlockIndex(depthStencilShader->ID, "Matrices");
	glUniformBlockBinding(depthStencilShader->ID, depthMatrices, 0);

	// set model mat for depthStencilShader
	glm::mat4 modelMatDepth = glm::mat4(1.0f);
	// cube coords are at -0.5~0.5; translate it to 0~1
	modelMatDepth = glm::translate(modelMatDepth, glm::vec3(0.5));
	depthStencilShader->use();
	depthStencilShader->setMat4("model", modelMatDepth);


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
			ImGui::SliderFloat("opacity", &opacity, 0.01f, 10.0f, "%.2f");
			ImGui::SliderInt("samples", &sampleCount, 10, 300);
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

		glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(camera->GetViewMat4()));
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(camera->GetProjectionMat4()));
		glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::vec3), glm::value_ptr(camera->GetCameraPos()));
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		
		renderToDepth();
		renderToTex();

		// bind textures on corresponding texture units
		// glActiveTexture(GL_TEXTURE0);
		// glBindTexture(GL_TEXTURE_3D, img3DTex);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, renderTexObj);

		drawShader->use();
		// render boxes
		glBindVertexArray(planeVAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// render imgui
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteBuffers(1, &uboMatrices);
	glDeleteFramebuffers(1, &framebufferObj);
	glDeleteRenderbuffers(1, &renderbufferObj);
	glDeleteTextures(1, &image3DTexObj);
	glDeleteTextures(1, &histogramTexObj);
	glDeleteTextures(1, &renderTexObj);
	glDeleteTextures(1, &textureColorbufferObj);


	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();

	//system("pause");
	return 0;
}