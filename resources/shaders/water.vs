#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

// Output to fragment shader
out vec2 fragTexCoord;
out vec3 fragPosition;
out vec3 fragNormal;

// Uniforms
uniform mat4 mvp;
uniform mat4 matModel;
uniform float time;

void main() {
    // Animate vertex Y position with waves
    vec3 pos = vertexPosition;
    
    // Multiple wave layers for organic look
    float wave1 = sin(pos.x * 2.0 + time * 1.5) * 0.1;
    float wave2 = sin(pos.z * 1.5 + time * 1.2) * 0.08;
    float wave3 = sin((pos.x + pos.z) * 3.0 + time * 2.0) * 0.05;
    
    pos.y += wave1 + wave2 + wave3;
    
    // Recalculate normal based on wave (approximation)
    float dx = cos(pos.x * 2.0 + time * 1.5) * 2.0 * 0.1 +
               cos((pos.x + pos.z) * 3.0 + time * 2.0) * 3.0 * 0.05;
    float dz = cos(pos.z * 1.5 + time * 1.2) * 1.5 * 0.08 +
               cos((pos.x + pos.z) * 3.0 + time * 2.0) * 3.0 * 0.05;
    
    vec3 waveNormal = normalize(vec3(-dx, 1.0, -dz));
    
    fragTexCoord = vertexTexCoord;
    fragPosition = vec3(matModel * vec4(pos, 1.0));
    fragNormal = waveNormal;
    
    gl_Position = mvp * vec4(pos, 1.0);
}
