#version 450

// Inputs from the vertex buffer
layout(location = 0) in vec3 inPosition;
layout(location = 1) in float inTexCoord;

// Output to the fragment shader
layout(location = 0) out float outTexCoord;

// Optional: Uniform buffer for transformation matrices
layout(binding = 0) uniform UniformBufferObject {
    mat4 model_matrix;
    mat4 view_matrix;
    mat4 proj_matrix;
} ubo;

void main() {
    // Transform the position and output it
    gl_Position = ubo.proj_matrix * ubo.view_matrix * ubo.model_matrix * vec4(inPosition, 1.0);    
    
    // Pass color map coordinate to fragment shader
    outTexCoord = inTexCoord;
}