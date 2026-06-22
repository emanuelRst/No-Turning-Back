#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D image;

void main() {
    vec2 uv = TexCoords;
    uv.y = 1.0 - uv.y;
    FragColor = texture(image, uv);
}
