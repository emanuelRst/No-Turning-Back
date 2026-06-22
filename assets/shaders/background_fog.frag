#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D image;
uniform float u_time;

void main() {
    vec2 uv = TexCoords;
    uv.y = 1.0 - uv.y;
    vec4 tex = texture(image, uv);
    float fog = sin(uv.x * 5.0 + u_time) * cos(uv.y * 5.0 + u_time * 0.5);
    fog = smoothstep(-0.5, 0.5, fog) * 0.3;
    vec3 color = mix(tex.rgb, vec3(0.7, 0.65, 0.55), fog);
    FragColor = vec4(color, tex.a);
}
