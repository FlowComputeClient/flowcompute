#version 450

// Catch the centroid qualifier
layout(location = 0) centroid in vec3 fragPosition;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    vec4 color;
} pc;

void main() {
    // Hardcoded suitable values for testing
    vec3 lightPos = vec3(5.0, 8.0, 5.0);
    vec3 lightColor = vec3(0.75, 0.75, 0.75); 
    vec3 objectColor = pc.color.rgb;

    // 1. Calculate Light Direction FIRST
    vec3 L = normalize(lightPos - fragPosition);

    // 2. Compute the geometric normal using partial derivatives
    vec3 dX = dFdx(fragPosition);
    vec3 dY = dFdy(fragPosition);
    vec3 crossProduct = cross(dX, dY);

    // 3. Protect against NaN. Fallback to L to prevent dark spots.
    vec3 flatNormal = (length(crossProduct) > 0.000001) ? normalize(crossProduct) : L;

    // Diffuse (Lambert) using the calculated flat normal
    float diff = max(dot(flatNormal, L), 0.0);

    // Ambient term (simple constant)
    float ambientStrength = 0.8;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse contribution
    vec3 diffuse = diff * lightColor;

    // Final color
    vec3 result = (ambient + diffuse) * objectColor;

    outColor = vec4(result, 1.0);
}