#version 420 core
#define POSITION 0
#define SCREEN_DIMENSIONS vec2(1024,768)

layout (location = 0, index = 0) out vec4 fragColor;

layout(binding = 0) uniform sampler3D testTexture;  

void main()
{
	// Based on the screen coordinates, sample the front-facing layer of the 3D texture
	vec3 textureIndex = vec3(gl_FragCoord.xy/SCREEN_DIMENSIONS, 0.0);
	fragColor = texture(testTexture, textureIndex);
}