#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D image;

void main() {
    FragColor = texture(image, vec2(TexCoords.x, 1.0 - TexCoords.y));
}
