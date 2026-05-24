#version 450

// Input variable passed from the vertex shader
layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    // Uses the interpolated color from the vertex stage
    outColor = vec4(fragColor, 1.0);
}