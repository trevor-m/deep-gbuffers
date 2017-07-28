#version 330 core
in vec2 TexCoords;
out vec3 color;

uniform sampler2D bufferInput;

const int blurKernelSize = 7;


void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(bufferInput, 0));
    vec3 result = vec3(0,0,0);

	int blurOffset = blurKernelSize/2;

	//apply box blur (average)
    for (int x = -blurOffset; x <= blurOffset; x++) {
        for (int y = -blurOffset; y <= blurOffset; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(bufferInput, TexCoords + offset).rgb;
        }
    }
    color = result / (blurKernelSize * blurKernelSize);
}  