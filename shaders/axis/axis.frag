#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 0) out vec4 outColor;

// Push constant for the scale
layout(push_constant) uniform PushConstants {
    float gridScale;
} pc;

void main() {
    float scale = max(pc.gridScale, 0.001); 

    vec4 gridColor = vec4(0.4, 0.4, 0.4, 1.0);
    vec4 xAxisColor = vec4(1.0, 0.2, 0.2, 1.0);
    vec4 yAxisColor = vec4(0.2, 1.0, 0.2, 1.0);

    // --- Grid Generation ---
    vec2 coord = inWorldPos.xy / scale;
    vec2 gridDeriv = fwidth(coord);
    vec2 gridLines = abs(fract(coord - 0.5) - 0.5) / gridDeriv;
    
    float lineAlpha = 1.0 - min(min(gridLines.x, gridLines.y), 1.0);

    // --- Axis Generation ---
    vec2 axisDeriv = fwidth(inWorldPos.xy);
    vec2 axisLines = abs(inWorldPos.xy) / axisDeriv;
    float distToXAxis = axisLines.y;
    float distToYAxis = axisLines.x;

    vec4 finalColor = vec4(gridColor.rgb, lineAlpha * 0.4);

    if (distToXAxis < 1.0) {
        finalColor = mix(finalColor, xAxisColor, 1.0 - distToXAxis);
    }
    
    if (distToYAxis < 1.0) {
        finalColor = mix(finalColor, yAxisColor, 1.0 - distToYAxis);
    }

    if (finalColor.a < 0.01) {
        discard;
    }

    outColor = finalColor;
}