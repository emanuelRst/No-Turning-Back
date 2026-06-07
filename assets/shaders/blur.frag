#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D image;
uniform float blurSize;

void main() {
    vec4 color = vec4(0.0);
    float total = 0.0;
    
    // Simple 3x3 box blur
    for (float x = -1.0; x <= 1.0; x += 1.0) {
        for (float y = -1.0; y <= 1.0; y += 1.0) {
            color += texture(image, TexCoords + vec2(x, y) * blurSize);
            total += 1.0;
        }
    }
    FragColor = color / total;
}
