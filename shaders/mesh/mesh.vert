#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inBarycentric;

// Outputs to the fragment shader
layout(location = 0) centroid out vec3 fragPosition;
layout(location = 1) out vec3 fragBarycentric;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model_matrix;
    mat4 view_matrix;
    mat4 proj_matrix;
} ubo;

void main() {
    gl_Position = ubo.proj_matrix * ubo.view_matrix * ubo.model_matrix * vec4(inPosition, 1.0);
    fragPosition = vec3(ubo.model_matrix * vec4(inPosition, 1.0));
    
    // Pass the coordinates straight through
    fragBarycentric = inBarycentric; 
}