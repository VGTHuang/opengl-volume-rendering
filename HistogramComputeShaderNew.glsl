// https://antongerdelan.net/opengl/compute.html#:~:text=Execution%201%20Creating%20the%20Texture%20%2F%20Image.%20We,...%205%20A%20Starter%20Ray%20Tracing%20Scene.%20
// https://stackoverflow.com/questions/45282300/writing-to-an-empty-3d-texture-in-a-compute-shader

#version 430
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(r16, binding = 0) uniform readonly image3D img_input;
layout(rgba32f, binding = 1) uniform image2D img_output;

uniform int maxImgValue;

float getImageData(ivec3 coords) {
	return imageLoad(img_input, coords).r * 65536.0 / float(maxImgValue);
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

float getImageLap(ivec3 coords) {
	float v = getImageData(coords);
	float v1 = getImageData(coords + ivec3(-1, 0, 0));
	float v2 = getImageData(coords + ivec3(1, 0, 0));
	float v3 = getImageData(coords + ivec3(0, -1, 0));
	float v4 = getImageData(coords + ivec3(0, 1, 0));
	float v5 = getImageData(coords + ivec3(0, 0, -1));
	float v6 = getImageData(coords + ivec3(0, 0, 1));
	float divX = v1 + v2 - 2.0 * v;
	float divY = v3 + v4 - 2.0 * v;
	float divZ = v5 + v6 - 2.0 * v;
	return 0.5 + divX + divY + divZ;
}

void main() {

  ivec3 pixel_coords = ivec3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

  float value =  getImageData(pixel_coords);
  float grad = getImageGrad(pixel_coords);

  ivec2 out_size = imageSize(img_output);
  ivec2 out_coords = ivec2(floor(out_size.x * value), floor(out_size.y * grad));
  
  float original_value = imageLoad(img_output, out_coords).x;
  float increment = pow(100, -1 / (1 - 1.1 * original_value));

  imageStore(img_output, out_coords, vec4(vec3(original_value + increment), 1.0));
  // imageStore(img_output, out_coords, vec4(1.0, 1.0, 0.0, 1.0));

}