// https://antongerdelan.net/opengl/compute.html#:~:text=Execution%201%20Creating%20the%20Texture%20%2F%20Image.%20We,...%205%20A%20Starter%20Ray%20Tracing%20Scene.%20
// https://stackoverflow.com/questions/45282300/writing-to-an-empty-3d-texture-in-a-compute-shader

#version 430
layout(local_size_x = 1, local_size_y = 1) in;

layout(rgba32f, binding = 0) uniform image2D img_input;
layout(rgba32f, binding = 1) uniform image2D img_output;

void main() {



  // base pixel colour for image
  vec4 pixel = vec4(1.0, 0.0, 0.0, 1.0);
  // get index in global work group i.e x,y position
  ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

  vec4 la = imageLoad(img_input, pixel_coords);
  
  //
  // interesting stuff happens here later
  //
  
  // output to a specific pixel in the image
  imageStore(img_output, pixel_coords, la);
}