#version 450

layout(location = 0) in vec3 fragPosition;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    vec4 color;
} pc;

void main() {

    // 1. Compute the geometric normal using partial derivatives
    vec3 dX = dFdx(fragPosition);
    vec3 dY = dFdy(fragPosition);
    vec3 flatNormal = normalize(cross(dX, dY));

    // Hardcoded suitable values for testing
    vec3 lightPos = vec3(10.0, 20.0, 10.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0); 
    vec3 objectColor = pc.color.rgb;

    // Light direction
    vec3 L = normalize(lightPos - fragPosition);

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