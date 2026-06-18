#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D image;
uniform float u_time;

void main() {
    vec2 uv = TexCoords;
    uv.y = 1.0 - uv.y; // Flip y to match original

    // Basic atmospheric movement (dusty fog effect)
    float fog = sin(uv.x * 5.0 + u_time) * cos(uv.y * 5.0 + u_time * 0.5);
    fog = smoothstep(-0.5, 0.5, fog) * 0.3; // Make it softer and blendable

    vec4 texColor = texture(image, uv);
    // Blend: mix texture with a subtle dusty color
    FragColor = vec4(mix(texColor.rgb, vec3(0.7, 0.65, 0.55), fog), texColor.a);
}
