#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTex;

uniform mat4 model;

layout (std140) uniform Matrices
{
	mat4 view;
	mat4 projection;
	vec3 camPos;
};

out vec3 vsOutPos;
out vec3 vsOutCamPos;
out vec3 vsOutNormal;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
	vsOutPos = vec3(model * vec4(aPos, 1.0f));
	vsOutCamPos = camPos;
	vsOutNormal = aNormal;
}