#version 450

layout(location = 0) centroid in vec3 fragPosition;
layout(location = 1) in vec3 fragBarycentric;

layout(location = 0) out vec4 outColor;

// Expanded push constants to control the wireframe
layout(push_constant) uniform PushConstants {
    vec4 color;
} pc;

void main() {
    vec3 lightPos = vec3(5.0, 8.0, 5.0);
    vec3 lightColor = vec3(0.75, 0.75, 0.75); 
    vec3 objectColor = pc.color.rgb;
    vec4 wireColor = vec4(0.95, 0.95, 0.83, 1.0);
    float wireThickness = 1.0;
    float enableWireframe = 1.0;
    float wireOpacity = 0.9;    

    vec3 L = normalize(lightPos - fragPosition);
    vec3 dX = dFdx(fragPosition);
    vec3 dY = dFdy(fragPosition);
    vec3 crossProduct = cross(dX, dY);

    vec3 flatNormal = (length(crossProduct) > 0.000001) ? normalize(crossProduct) : L;
    float diff = max(dot(flatNormal, L), 0.0);
    
    float ambientStrength = 0.8;
    vec3 ambient = ambientStrength * lightColor;
    vec3 diffuse = diff * lightColor;
    
    vec3 shadedColor = (ambient + diffuse) * objectColor;
    
    // Calculate the rate of change of the barycentric coordinates across pixels
    vec3 d = fwidth(fragBarycentric);
    
    // Smoothstep creates a smooth gradient at the line edges for anti-aliasing.
    // Multiplying 'd' by the thickness expands the boundary of the line.
    vec3 a3 = smoothstep(vec3(0.0), d * wireThickness, fragBarycentric);
    
    // The edge factor is the minimum of the three components. 
    // It approaches 0.0 at the edges and is 1.0 in the center of the triangle.
    float edgeFactor = min(min(a3.x, a3.y), a3.z);
    
    // Final Color Mixing ---
    if (enableWireframe > 0.5) {
        vec3 finalColor = mix(shadedColor, wireColor.rgb, (1.0 - edgeFactor) * wireOpacity);
        outColor = vec4(finalColor, 1.0);
    } else {
        // Just output the solid flat-shaded color
        outColor = vec4(shadedColor, 1.0);
    }
}