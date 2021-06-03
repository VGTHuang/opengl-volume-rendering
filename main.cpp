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

std::string getImage3DConfig(int &x, int &y, int &z) {
	std::string data;
	std::ifstream rfile;

	rfile.open("file_config.txt", std::ios::in | std::ios::out);

	rfile >> data;

	rfile >> x >> y >> z;

	rfile.close();

	return data;
}

GLuint tex_input, tex_output;
void genHistogramThisFunctionActuallyWorks(unsigned int img3DTex, glm::ivec3 img3DShape) {
	// https://antongerdelan.net/opengl/compute.html#:~:text=Execution%201%20Creating%20the%20Texture%20%2F%20Image.%20We,...%205%20A%20Starter%20Ray%20Tracing%20Scene.%20

	// input
	int in_tex_w = 512, in_tex_h = 512;
	float *f = new float[in_tex_w * in_tex_h * 4];
	for (int i = 0; i < in_tex_w * in_tex_h; i++) {
		f[i * 4 + 1] = 1.0;
	}
	glGenTextures(1, &tex_input);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_input);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, in_tex_w, in_tex_h, 0, GL_RGBA, GL_FLOAT,
		f);
	glBindImageTexture(0, tex_input, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);


	// output
	int out_tex_w = 512, out_tex_h = 512;

	glGenTextures(1, &tex_output);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_output);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, out_tex_w, out_tex_h, 0, GL_RGBA, GL_FLOAT,
		NULL);
	glBindImageTexture(1, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);


	histogramComputeShader->use();
	glDispatchCompute(256, 128, 1);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}


void genHistogram(unsigned short *imgVals, glm::ivec3 img3DShape) {
	// input
	glGenTextures(1, &tex_input);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, tex_input);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R16, img3DShape.y, img3DShape.z, img3DShape.x, 0, GL_RED, GL_UNSIGNED_SHORT,
		imgVals);
	glBindImageTexture(0, tex_input, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16);



	// output
	int out_tex_w = 512, out_tex_h = 512;

	glGenTextures(1, &tex_output);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_output);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, out_tex_w, out_tex_h, 0, GL_RGBA, GL_FLOAT,
		NULL);
	glBindImageTexture(1, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);


	histogramComputeShader->use();
	glDispatchCompute(img3DShape.y, img3DShape.z, img3DShape.x);

	// glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);


	float *new_array = (float *)malloc(sizeof(float) * 4 * out_tex_w * out_tex_h);
	glBindTexture(GL_TEXTURE_2D, tex_output);

	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, new_array);

	int asd = glGetError();
	std::vector<float> vvv;
	for (int i = 0; i < 100; i++) {
		vvv.push_back(new_array[i]);
	}
	// WE HAVE IT! save array to bmp?

	int aasdfgasdfg = 1;
}


// int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
int main()
{
	// config
	std::string path(getImage3DConfig(imageX, imageY, imageZ));

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


	// ******************************* read medical data *******************************

	glm::ivec3 imgShape(imageX, imageY, imageZ);
	FILE *fpsrc = NULL;
	unsigned short *imgValsUINT;
	imgValsUINT = (unsigned short *)malloc(sizeof(unsigned short) * imageX * imageY * imageZ);
	errno_t err = fopen_s(&fpsrc, path.c_str(), "r");
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
	genHistogram(imgValsUINT, imgShape);

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
				printf("btn clicked\n");
			}
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
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(camera->GetViewMat4()));
		glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(camera->GetProjectionMat4()));
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		// bind textures on corresponding texture units
		// glActiveTexture(GL_TEXTURE0);
		// glBindTexture(GL_TEXTURE_3D, img3DTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex_output);

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