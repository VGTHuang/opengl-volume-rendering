#version 430 core
out vec4 FragColor;

in vec2 vsOutTexCoord;

uniform sampler2D texFromCompute;

void main()
{
	FragColor = texture(texFromCompute, vsOutTexCoord);
}