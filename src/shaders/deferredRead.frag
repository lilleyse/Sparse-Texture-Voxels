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


//---------------------------------------------------------
// SHADER CONSTANTS
//---------------------------------------------------------

#define EPS       0.0001
#define PI        3.14159265
#define HALFPI    1.57079633
#define ROOTTHREE 1.73205081

#define EQUALS(A,B) ( abs((A)-(B)) < EPS )
#define EQUALSZERO(A) ( ((A)<EPS) && ((A)>-EPS) )


//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout(location = 0) out vec4 fragColor;

layout(binding = DEFERRED_POSITIONS_TEXTURE_BINDING) uniform sampler2D tPosition;
layout(binding = DEFERRED_COLORS_TEXTURE_BINDING) uniform sampler2D tColor;
layout(binding = DEFERRED_NORMALS_TEXTURE_BINDING) uniform sampler2D tNormal;

layout(binding = COLOR_TEXTURE_3D_BINDING) uniform sampler3D tVoxColor;

in vec2 vUV;

uniform float uTextureRes;

const int NUM_CONES = 5;
const int MAX_STEPS = 128;
const float STEPSIZE_WRT_TEXEL = 1.0;  // Cyrill uses 1/3
const float ALPHA_THRESHOLD = 0.95;

float gTexelSize;


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

// special case, optimized for 0.0 to 1.0
bool textureVolumeIntersect(vec3 ro, vec3 rd, out float t) {    
    vec3 tMin = -ro / rd;
    vec3 tMax = (1.0-ro) / rd;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    
    if (tNear<tFar && tFar>0.0) {
        // difference here
        // if inside, instead of returning far plane, return ray origin
        t = tNear>0.0 ? tNear : 0.0;
        return true;
    }
    
    return false;
}

// simple alpha blending
vec4 conetraceSimple(vec3 ro, vec3 rd, float fov) {
  vec3 pos = ro;
  
  vec4 color = vec4(0.0);
  
  for (int i=0; i<MAX_STEPS; ++i) {
    float dist = distance(pos, uCamPos);

    // size of texel cube we want to be looking into
    // correctly interpolated texel size, automatic
    float pixSize =  dist * tan(fov/180.0*PI);
    
    // calc mip size
    // if pixSize smaller than texel, clamp. that's the smallest we can go
    float mipLevel;
    if (pixSize > gTexelSize) {
        // solve: pixSize = texelSize*2^mipLevel
        mipLevel = log2(pixSize/gTexelSize);
    }
    else {
        mipLevel = 0.0;
        pixSize = gTexelSize;
    }

    // take step relative to the interpolated size
    float stepSize = pixSize * STEPSIZE_WRT_TEXEL;

    // sample texture
    vec4 src = textureLod(tVoxColor, pos, mipLevel);
    
    // alpha normalized to 1 texel, i.e., 1.0 alpha is 1 solid block of texel
    // no need weight by "stepSize" since "pixSize" is size of an imaginary 
    // texel cube exactly the size of a mip cube we want, if it existed, 
    // but it doesn't so we interpolate between two mips to approximate it
    // but need to weight by stepsize within texel
    src.a *= STEPSIZE_WRT_TEXEL;

    // alpha blending
    vec4 dst = color;
    color.a = src.a + dst.a*(1.0-src.a);
    color.rgb = EQUALSZERO(color.a) ? vec3(0.0) : 
        (src.rgb*src.a + dst.rgb*dst.a*(1.0-src.a)) / color.a;

    pos += stepSize*rd;
    
    if (color.a > ALPHA_THRESHOLD ||
      pos.x > 1.0 || pos.x < 0.0 ||
      pos.y > 1.0 || pos.y < 0.0 ||
      pos.z > 1.0 || pos.z < 0.0)
      break;
  }
  
  return color;
}

void main()
{
    // size of one texel in normalized texture coords
    gTexelSize = 1.0/uTextureRes;

    vec3 pos = texture(tPosition, vUV).rgb;
    vec3 nor = texture(tNormal, vUV).rgb;
    vec4 col = texture(tColor, vUV);

    vec3 V = normalize(pos-uCamPos);
    V = reflect(V, nor);
    const float fov = 0.1;

    // output color
    vec4 cout;

    float t;
    if ( col.a!=0.0 && textureVolumeIntersect(pos, V, t) )
        cout = conetraceSimple(pos+V*(t+EPS), V, fov);
    else
        cout = vec4(0.0);

    // background color
    vec4 bg = vec4(vec3((1.0-vUV.y)/2.0), 1.0);

    // alpha blend cout over bg
    bg.rgb = mix(bg.rgb, cout.rgb, cout.a);
    fragColor = bg;
}