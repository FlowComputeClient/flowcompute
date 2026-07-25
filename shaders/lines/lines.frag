#version 450

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    vec4 color;
} pc;

void main() {
    // Calculate perceptual luminance using the RGB values of the push constant
    float lum = dot(pc.color.rgb, vec3(0.299, 0.587, 0.114));
    
    // Use a steeper mix for better contrast on thin lines
    vec3 lineColor = (lum < 0.6) 
        ? mix(pc.color.rgb, vec3(1.0), 0.5)  // 50% mix toward white for dark patches
        : pc.color.rgb * 0.35;               // 65% darker for bright patches
        
    outColor = vec4(lineColor, 1.0);
}