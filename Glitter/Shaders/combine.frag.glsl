#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D bufferRadiosity;
uniform sampler2DArray bufferColor;

const int blurKernelSize = 4;


void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(bufferRadiosity, 0));
    vec3 result = vec3(0,0,0);

	int blurOffset = blurKernelSize/2;

	//apply box blur (average)
    for (int x = -blurOffset; x < blurOffset; x++) {
        for (int y = -blurOffset; y < blurOffset; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(bufferRadiosity, TexCoords + offset).rgb;
        }
    }
    color = vec4(result / (blurKernelSize * blurKernelSize) + texture(bufferColor, vec3(TexCoords, 0)).rgb, 1);
}  