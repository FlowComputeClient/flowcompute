#version 450

layout(location = 0) in vec3 inPosition;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model_matrix;
    mat4 view_matrix;
    mat4 proj_matrix;
} ubo;

void main() {
    gl_Position = ubo.proj_matrix * ubo.view_matrix * ubo.model_matrix * vec4(inPosition, 1.0);
}