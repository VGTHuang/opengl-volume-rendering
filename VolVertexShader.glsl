#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTex;

layout (std140) uniform Matrices
{
	uniform mat4 model;
    mat4 view;
    mat4 projection;
};

out vec3 vsOutNormal;
out vec4 vsOutPosition;
out vec2 vsOutTexCoord;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
	vsOutPosition = model * vec4(aPos, 1.0f);
	vsOutNormal = vec3(model * vec4(aNormal, 1.0));
	vsOutTexCoord = aTex;
}