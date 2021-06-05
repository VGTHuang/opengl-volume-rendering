// https://antongerdelan.net/opengl/compute.html#:~:text=Execution%201%20Creating%20the%20Texture%20%2F%20Image.%20We,...%205%20A%20Starter%20Ray%20Tracing%20Scene.%20
// https://stackoverflow.com/questions/45282300/writing-to-an-empty-3d-texture-in-a-compute-shader

#version 430
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(rgba32f, binding = 1) uniform image2D img_output;

uniform vec4 bg;

void main() {
  imageStore(img_output, ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y), bg);
}