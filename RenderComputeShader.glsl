#version 430
layout(local_size_x = 4, local_size_y = 4, local_size_z = 1) in;

layout(r16, binding = 1) uniform readonly image3D imgInput;
layout(rgba32f, binding = 2) uniform image2D imgOutput;

uniform sampler2D transfer;
uniform sampler2D depth;

uniform float canvasSize;
uniform int maxImgValue;
uniform mat4 model;
uniform vec4 bg;
uniform float opacity;
uniform int sampleCount;

float far = 100;
float near = 0.1;

layout (std140) uniform Matrices
{
	mat4 view;
	mat4 projection;
	vec3 camPos;
};

float getImageData(ivec3 coords) {
	return imageLoad(imgInput, coords).r * 65536.0 / float(maxImgValue);
}

float getImageGrad(ivec3 coords) {
	float v1 = getImageData(coords + ivec3(-1, 0, 0));
	float v2 = getImageData(coords + ivec3(1, 0, 0));
	float v3 = getImageData(coords + ivec3(0, -1, 0));
	float v4 = getImageData(coords + ivec3(0, 1, 0));
	float v5 = getImageData(coords + ivec3(0, 0, -1));
	float v6 = getImageData(coords + ivec3(0, 0, 1));
	return sqrt(pow((v1-v2)/2.0, 2) + pow((v3-v4)/2.0, 2) + pow((v5-v6)/2.0, 2));
}

vec4 getTransferedVal(ivec3 coords) {
  float value = getImageData(coords);
  float grad = getImageGrad(coords);

  // calc color
  vec2 transferCoords = vec2(value, grad);
  vec4 transferedColor = texture(transfer, transferCoords);

  return transferedColor;

}

// https://antongerdelan.net/opengl/raycasting.html
vec3 getRay(vec2 pxPos) {
	vec3 ray = (inverse(view) * inverse(projection) * vec4(pxPos * (far - near), far + near, far - near)).xyz;
	return ray;
}

// https://learnopengl.com/Advanced-OpenGL/Depth-testing
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}


void main() {
	/*
	// calc projected coords
	vec4 fakePosition = projection * view * model * vec4(float(gl_GlobalInvocationID.x) / 512.0, float(gl_GlobalInvocationID.y) / 512.0, float(gl_GlobalInvocationID.z) / 512.0, 1.0);
	vec2 fakeFragCoord = (1 + fakePosition.xy / fakePosition.w) * 512.0 / 2;

  
	// calc value & grad at sampled position of image3d
	ivec3 voxelCoords = ivec3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);
	vec4 transferedColor = getTransferedVal(voxelCoords);
	transferedColor.w *= 0.2;
	vec4 originalPxValue = imageLoad(imgOutput, ivec2(fakeFragCoord));
	vec4 blendColor = transferedColor *transferedColor.w + originalPxValue * (1.0 - transferedColor.w);
	blendColor.w = 1;

	if(fakePosition.w > 0)
		imageStore(imgOutput, ivec2(fakeFragCoord), blendColor);
	*/
	

	vec4 depthIndicator = texture(depth, vec2(float(gl_GlobalInvocationID.x) / canvasSize, float(gl_GlobalInvocationID.y) / canvasSize));
	/*
	// draw a red bbox of the rendered volume for testing
	vec4 originalPxValue1 = imageLoad(imgOutput, ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y));
	originalPxValue1 += depthIndicator * 0.2;
	imageStore(imgOutput, ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y), originalPxValue1);
	*/



	vec4 rayEnd = vec4(getRay(vec2(float(gl_GlobalInvocationID.x) / canvasSize, float(gl_GlobalInvocationID.y) / canvasSize) * 2 - 1), 0.0);
	vec4 rayStart = vec4(camPos, 0.0);
	vec3 ray = normalize(vec3(rayEnd - rayStart));
	float nearestVoxDist = LinearizeDepth(depthIndicator.x) - 0.2;
	if(depthIndicator.y > 0.1) {
		// camera is inside the volume
		nearestVoxDist = 0;
	}
	vec3 tempCastPos = camPos + ray * nearestVoxDist;

	// front to back compositing
	vec3 accC = vec3(0);
	float accA = 0;
	for(int i = 0; i < sampleCount; i++) {
		vec4 currentCol = getTransferedVal(ivec3(tempCastPos * canvasSize));
		// C'(i) = (1 - A'(i-1))C(i) + C'(i-1)
		accC = (1 - accA) * vec3(currentCol) + accC;
		// A'(i) = (1-A'(i-1)).A(i) + A'(i-1)
		accA = (1 - accA) * currentCol.w * opacity / sampleCount + accA;
		if(accA >= 0.5) {
			break;
		}
		tempCastPos += ray / (0.6 * sampleCount);
	}
	if(depthIndicator.z > 0.1)
		imageStore(imgOutput, ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y), vec4(vec3(bg) * (1 - accA) + accC * accA, 1));

	
	/*
	// draw a white dot at origin (0,0,0)
	vec4 zeroPosition1 = projection * view * (vec4(camPos/2, 1.0));
	vec2 zeroCoord1 = (1 + zeroPosition1.xy / zeroPosition1.w) * 512.0 / 2;
	if(zeroPosition1.w > 0)
	imageStore(imgOutput, ivec2(zeroCoord1), vec4(1.0, 1.0, 1.0, 0.0));
	*/
}