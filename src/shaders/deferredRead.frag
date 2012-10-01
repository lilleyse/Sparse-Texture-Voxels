//---------------------------------------------------------
// GLOBALS
//---------------------------------------------------------

#version 420 core
#define POSITION_ATTR 0
#define NORMAL_ATTR 1
#define DEBUG_TRANSFORM_ATTR 2
#define DEBUG_COLOR_ATTR 3
#define PER_FRAME_UBO_BINDING 0
#define COLOR_TEXTURE_3D_BINDING 0
#define NORMAL_TEXTURE_3D_BINDING 1
#define DEFERRED_POSITIONS_TEXTURE_BINDING 2
#define DEFERRED_COLORS_TEXTURE_BINDING 3
#define DEFERRED_NORMALS_TEXTURE_BINDING 4
#define DEFERRED_POSITIONS_FBO_BINDING 0
#define DEFERRED_COLORS_FBO_BINDING 1
#define DEFERRED_NORMALS_FBO_BINDING 2


layout(std140, binding = PER_FRAME_UBO_BINDING) uniform PerFrameUBO
{
    mat4 viewProjection;
    vec3 uCamLookAt;
    vec3 uCamPos;
    vec3 uCamUp;
    vec2 uResolution;
    float uAspect;
    float uTime;
    float uFOV;
};

layout(location = 0) out vec4 fragColor;

layout(binding = DEFERRED_POSITIONS_TEXTURE_BINDING) uniform sampler2D positionTexture;
layout(binding = DEFERRED_COLORS_TEXTURE_BINDING) uniform sampler2D colorTexture;
layout(binding = DEFERRED_NORMALS_TEXTURE_BINDING) uniform sampler2D normalTexture;

in vec2 vUV;

void main()
{
    // DEBUGTEST: manually init lights
    vec3 lightCol = vec3(1.0, 0.9, 0.8);
    vec3 lightPos = vec3(1.0, 2.0, 1.0);
    lightPos.x = 2.0*sin(uTime);
    lightPos.z = 2.0*cos(uTime);

    vec3 pos = texture(positionTexture, vUV).rgb;
    vec3 nor = texture(normalTexture, vUV).rgb;
    vec3 col = texture(colorTexture, vUV).rgb;

    const float KA = 0.2;
    const float KD = 0.8;

    vec3 toLight = normalize(lightPos-pos);
    col *= KA + KD*max(dot(toLight,nor),0.0);

    fragColor = vec4(col, 1.0);
}