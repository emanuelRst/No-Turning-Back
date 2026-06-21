#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D image;
uniform float u_time;

void main() {
    vec2 uv = TexCoords;
    uv.y = 1.0 - uv.y; // Flip y

    // 1. Chromatic Aberration
    float dist = 0.003;
    float r = texture(image, uv + vec2(dist, 0.0)).r;
    float g = texture(image, uv).g;
    float b = texture(image, uv - vec2(dist, 0.0)).b;
    float a = texture(image, uv).a;
    vec3 color = vec3(r, g, b);

    // 2. Existing Fog effect
    float fog = sin(uv.x * 5.0 + u_time) * cos(uv.y * 5.0 + u_time * 0.5);
    fog = smoothstep(-0.5, 0.5, fog) * 0.3;
    color = mix(color, vec3(0.7, 0.65, 0.55), fog);

    // 3. Scanlines
    float scanline = sin(uv.y * 800.0) * 0.05;
    color -= scanline;

    FragColor = vec4(color, a);
}
