#version 450

// Inputs from the vertex buffer
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 uvCoords;

// Output to the fragment shader
layout(location = 0) out vec3 fragColor;

// Optional: Uniform buffer for transformation matrices
layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp_matrix;
} ubo;

void main() {
    // Transform the position and output it
    gl_Position = ubo.mvp_matrix * vec4(inPosition, 1.0);
    
    // Pass color through to fragment shader
    fragColor = inColor;
}