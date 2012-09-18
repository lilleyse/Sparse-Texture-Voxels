#version 420 core

layout (location = 0, index = 0) out vec4 fragColor;

layout(std140, binding = PER_FRAME_UBO_BINDING) uniform PerFrameUBO
{
    mat4 viewProjection;
    vec3 uCamLookAt;
    vec3 uCamPosition;
    vec3 uCamUp;
    uvec2 uResolution;
};

in block
{
    flat vec4 color;
} vertexData;

void main()
{
    fragColor = vertexData.color;
}