#version 430 core
out vec4 FragColor;

in vec3 vsOutPos;
in vec3 vsOutCamPos;
in vec3 vsOutNormal;

void main()
{
	vec3 camDir = vsOutCamPos - vsOutPos;
	float isBack = 0;
	if(dot(camDir, vsOutNormal) < 0) {
		isBack = 1;
	}
	FragColor = vec4(gl_FragCoord.z, isBack, 1.0, 1.0);
}