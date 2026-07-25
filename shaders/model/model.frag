#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    vec4 color;
} pc;

void main() {
    vec3 objectColor = pc.color.rgb;

    // 1. Compute scale-independent geometric normals
    // Normalize the screen-space derivatives FIRST. This removes the magnitude 
    // of the coordinate system (meters vs mm), leaving only the pure direction.
    vec3 dX = normalize(dFdx(fragPosition));
    vec3 dY = normalize(dFdy(fragPosition));

    // The cross product of two normalized vectors is safe from quadratic underflow.
    // Adding a tiny fallback prevents NaN if the face is perfectly edge-on to the camera.
    vec3 crossProduct = cross(dX, dY);
    float sqLength = dot(crossProduct, crossProduct);
    vec3 flatNormal = (sqLength > 1e-8) ? normalize(crossProduct) : vec3(0.0, 0.0, 1.0);

    // 2. Define Light Position relative to the fragment
    // For a simple visualization client, a directional light matching the camera 
    // view direction (headlight) ensures the model is always lit, regardless of scale.
    // If fragPosition is in View Space, the camera is at (0,0,0), so L is just -normalize(fragPosition).
    // If fragPosition is in World Space, we can fake a high overhead directional light for now:
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5)); 

    // Diffuse (Lambert) using the calculated flat normal
    float diff = max(dot(flatNormal, lightDir), 0.0);

    // Ambient and Diffuse calculations
    vec3 lightColor = vec3(0.75, 0.75, 0.75); 
    float ambientStrength = 0.7;
    
    vec3 ambient = ambientStrength * lightColor;
    vec3 diffuse = diff * lightColor;

    // Final color
    vec3 result = (ambient + diffuse) * objectColor;

    outColor = vec4(result, 1.0);
}