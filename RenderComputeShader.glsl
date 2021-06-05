// https://antongerdelan.net/opengl/compute.html#:~:text=Execution%201%20Creating%20the%20Texture%20%2F%20Image.%20We,...%205%20A%20Starter%20Ray%20Tracing%20Scene.%20
// https://stackoverflow.com/questions/45282300/writing-to-an-empty-3d-texture-in-a-compute-shader

#version 430
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(r16, binding = 1) uniform readonly image3D img_input;
layout(rgba32f, binding = 2) uniform image2D img_output;

uniform sampler2D transfer;

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


void main() {

  ivec3 pixel_coords = ivec3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

  float value =  getImageData(pixel_coords);
  float grad = getImageGrad(pixel_coords);

  ivec2 out_size = imageSize(img_output);
  vec2 out_coords = vec2(value, grad);

  vec4 out_color = texture(transfer, out_coords);
  //vec4 out_color = vec4(1.0, 0.0, 0.0, 1.0);

  vec4 original_value = imageLoad(img_output, ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y));

  vec4 blend_color = out_color * out_color.w + original_value * (1.0 - out_color.w);
  blend_color.w = 1;

  imageStore(img_output, ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y), blend_color);

}