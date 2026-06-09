#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D screenTexture;

void main() {
    vec2 texelSize = 1.0 / textureSize(screenTexture, 0);
    vec4 color = vec4(0.0);
    float kernel[9] = float[](
        1, 2, 1,
        2, 4, 2,
        1, 2, 1
    );
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            color += texture(screenTexture, TexCoords + offset) * kernel[(x+1) + (y+1)*3];
        }
    }
    color /= 16.0;
    FragColor = color;
}
