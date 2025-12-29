#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec3 fragPosition;
in vec3 fragNormal;

// Output
out vec4 finalColor;

// Uniforms
uniform float time;
uniform vec3 viewPos;      // Camera position
uniform vec4 colDiffuse;   // Base color from raylib

void main() {
    // Water base colors
    vec3 deepColor = vec3(0.1, 0.3, 0.5);    // Deep blue
    vec3 shallowColor = vec3(0.3, 0.6, 0.7); // Lighter blue-green
    vec3 foamColor = vec3(0.9, 0.95, 1.0);   // White foam
    
    // Simple fresnel for edge highlighting
    vec3 viewDir = normalize(viewPos - fragPosition);
    float fresnel = pow(1.0 - max(dot(fragNormal, viewDir), 0.0), 3.0);
    
    // Mix deep and shallow based on fresnel
    vec3 waterColor = mix(deepColor, shallowColor, fresnel * 0.5);
    
    // Add subtle foam on wave peaks (based on normal Y)
    float foam = smoothstep(0.85, 0.95, fragNormal.y);
    waterColor = mix(waterColor, foamColor, foam * 0.3);
    
    // Simple specular highlight
    vec3 lightDir = normalize(vec3(1.0, 1.0, 0.5));
    vec3 reflectDir = reflect(-lightDir, fragNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    waterColor += vec3(1.0) * spec * 0.5;
    
    // Transparency
    float alpha = 0.8 + fresnel * 0.2;
    
    finalColor = vec4(waterColor, alpha);
}
