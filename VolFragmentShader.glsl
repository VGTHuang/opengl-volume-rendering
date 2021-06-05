#version 430 core
out vec4 FragColor;

in vec3 vsOutNormal;
in vec4 vsOutPosition;
in vec2 vsOutTexCoord;

uniform vec3 camPos;

uniform sampler2D texFromCompute;

void main()
{
	// float intensity = texture(image3DTex, vec3(vsOutTexCoord.x, vsOutTexCoord.y, 0.2)).r;
	FragColor = texture(texFromCompute, vsOutTexCoord);
	// FragColor = vec4(intensity, intensity, intensity, 1.0);
}