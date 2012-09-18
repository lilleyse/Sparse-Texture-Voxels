#version 420 core
#define POSITION_ATTR 0
#define DEBUG_TRANSFORM_ATTR 1
#define DEBUG_COLOR_ATTR 2
#define PER_FRAME_UBO_BINDING 0

layout (location = 0, index = 0) out vec4 fragColor;

layout(binding = 0) uniform sampler3D testTexture;

layout(std140, binding = PER_FRAME_UBO_BINDING) uniform PerFrameUBO
{
    mat4 viewProjection;
    vec3 uCamLookAt;
    vec3 uCamPosition;
    vec3 uCamUp;
    uvec2 uScreenDim;
};

void main()
{
    vec2 uv = gl_FragCoord.xy/uScreenDim;
    uv.y = 1.0-uv.y;
    
    /* CAMERA RAY */
    vec3 C = normalize(uCamLookAt-uCamPosition);
    vec3 A = normalize(cross(C,uCamUp));
    vec3 B = -(float(uScreenDim.y)/float(uScreenDim.x))*normalize(cross(A,C));
    
    // scale A and B by root3/3 : fov = 30 degrees
    vec3 ro = uCamPosition+C + (2.0*uv.x-1.0)*0.57735027*A + (2.0*uv.y-1.0)*0.57735027*B;
    vec3 rd = normalize(ro-uCamPosition);
    
    // Based on the screen coordinates, sample the front-facing layer of the 3D texture
    // vec3 textureIndex = vec3(uv, 0.0);
    // fragColor = texture(testTexture, textureIndex);
    
    fragColor = vec4(rd, 1.0);
}