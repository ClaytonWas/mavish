#version 330

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float time;

out vec4 finalColor;

// Posterize colors for that comic/Moebius look
vec3 posterize(vec3 color, float levels) {
    return floor(color * levels) / levels;
}

// Sobel edge detection on luminance
float sobel(vec2 uv, vec2 texel) {
    float tl = dot(texture(texture0, uv + vec2(-1, -1) * texel).rgb, vec3(0.299, 0.587, 0.114));
    float tm = dot(texture(texture0, uv + vec2( 0, -1) * texel).rgb, vec3(0.299, 0.587, 0.114));
    float tr = dot(texture(texture0, uv + vec2( 1, -1) * texel).rgb, vec3(0.299, 0.587, 0.114));
    float ml = dot(texture(texture0, uv + vec2(-1,  0) * texel).rgb, vec3(0.299, 0.587, 0.114));
    float mr = dot(texture(texture0, uv + vec2( 1,  0) * texel).rgb, vec3(0.299, 0.587, 0.114));
    float bl = dot(texture(texture0, uv + vec2(-1,  1) * texel).rgb, vec3(0.299, 0.587, 0.114));
    float bm = dot(texture(texture0, uv + vec2( 0,  1) * texel).rgb, vec3(0.299, 0.587, 0.114));
    float br = dot(texture(texture0, uv + vec2( 1,  1) * texel).rgb, vec3(0.299, 0.587, 0.114));
    
    float gx = -tl - 2.0*ml - bl + tr + 2.0*mr + br;
    float gy = -tl - 2.0*tm - tr + bl + 2.0*bm + br;
    
    return sqrt(gx*gx + gy*gy);
}

void main() {
    vec2 texel = 1.0 / resolution;
    vec2 uv = fragTexCoord;
    
    // Get scene color
    vec3 color = texture(texture0, uv).rgb;
    
    // Posterize for flat color bands
    vec3 posterized = posterize(color, 8.0);
    
    // Edge detection
    float edge = sobel(uv, texel);
    
    // Smooth edge line
    float line = smoothstep(0.1, 0.3, edge);
    
    // Dark ink outline
    vec3 inkColor = vec3(0.1, 0.08, 0.06);
    vec3 result = mix(posterized, inkColor, line);
    
    finalColor = vec4(result, 1.0);
}
