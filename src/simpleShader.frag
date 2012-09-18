/******************  GLOBALS  **********************/

#version 420 core
#define POSITION_ATTR 0
#define DEBUG_TRANSFORM_ATTR 1
#define DEBUG_COLOR_ATTR 2
#define PER_FRAME_UBO_BINDING 0
#define VOXEL_TEXTURE_3D_BINDING 0


layout(std140, binding = PER_FRAME_UBO_BINDING) uniform PerFrameUBO
{
    mat4 viewProjection;
    vec3 uCamLookAt;
    vec3 uCamPosition;
    vec3 uCamUp;
    uvec2 uResolution;
    float uTime;
};

/***************************************************/


layout (location = 0, index = 0) out vec4 fragColor;
layout(binding = VOXEL_TEXTURE_3D_BINDING) uniform sampler3D testTexture;


void main()
{
    float aspect = float(uResolution.x)/float(uResolution.y);
    vec2 uv = gl_FragCoord.xy/uResolution;
    uv.y = 1.0-uv.y;
    
    ///* CAMERA RAY */
    //vec3 C = normalize(uCamLookAt-uCamPosition);
    //vec3 A = normalize(cross(C,uCamUp));
    //vec3 B = -1.0/(aspect)*normalize(cross(A,C));
    //
    //// scale A and B by root3/3 : fov = 30 degrees
    //vec3 ro = uCamPosition+C + (2.0*uv.x-1.0)*0.57735027*A + (2.0*uv.y-1.0)*0.57735027*B;
    //vec3 rd = normalize(ro-uCamPosition);
    //
    //fragColor = vec4(rd, 1.0);
    
    // Based on the screen coordinates, sample the front-facing layer of the 3D texture
    uv.x *= aspect;  // correct to square
    vec3 textureIndex = vec3(uv, fract(uTime));
    fragColor = texture(testTexture, textureIndex);
}