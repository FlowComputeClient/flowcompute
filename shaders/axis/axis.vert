#version 450

layout(location = 0) in vec3 inPosition;

// Output world position to the fragment shader to calculate the grid
layout(location = 0) out vec3 outWorldPos;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model_matrix;
    mat4 view_matrix;
    mat4 proj_matrix;
} ubo;

void main() {
    // Transform the vertex to world space
    vec4 worldPos = ubo.model_matrix * vec4(inPosition, 1.0);
    outWorldPos = worldPos.xyz;
    
    // Project to screen space
    gl_Position = ubo.proj_matrix * ubo.view_matrix * worldPos;
}