//---------------------------------------------------------
// GLOBALS
//---------------------------------------------------------

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
    vec2 uResolution;
    float uTime;
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

layout (location = 0, index = 0) out vec4 fragColor;
layout(binding = VOXEL_TEXTURE_3D_BINDING) uniform sampler3D testTexture;

const int MAX_STEPS = 128;
const float ALPHA_THRESHOLD = 0.95;

// needs to be uniforms
const float uFOV = 30.0f;
const float uVoxelRes = 256.0f;

float gVoxelSize;
float gMaxMipLevel;
float gMaxMipDist;
float gPixSizeAtDist;

// DEBUGTEST: change to uniform later
const int LIGHT_NUM = 1;
vec3 gLightPos[LIGHT_NUM];
vec3 gLightCol[LIGHT_NUM];


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

// cube intersect
bool cubeIntersect(vec3 bMin, vec3 bMax, vec3 ro, vec3 rd, out float t) {    
    vec3 tMin = (bMin-ro) / rd;
    vec3 tMax = (bMax-ro) / rd;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    
    if (tNear<tFar && tFar>0.0) {
        t = tNear>0.0 ? tNear : tFar;
        return true;
    }
    
    return false;
}

// cube intersect, but t returns intersect of volume, not just sides
bool cubeVolumeIntersect(vec3 bMin, vec3 bMax, vec3 ro, vec3 rd, out float t) {    
    vec3 tMin = (bMin-ro) / rd;
    vec3 tMax = (bMax-ro) / rd;
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
vec4 conetraceSimple(vec3 ro, vec3 rd) {
  vec3 pos = ro;
  
  vec4 color = vec4(0.0);
  
  for (int i=0; i<MAX_STEPS; ++i) {
    float dist = min( distance(pos, uCamPosition), gMaxMipDist );
    float mipLevel = gMaxMipLevel * dist/gMaxMipDist;

    //float mipSize = 1.0 / pow(2,(gMaxMipLevel-mipLevel));  //TODO
    float stepSize = 0.01;//mipSize/3.0;

    vec4 src = vec4( vec3(1.0), textureLod(testTexture, pos, mipLevel).r );
    //src.a *= gStepSize;  // factor by how steps per voxel diag
    //src.a *= stepSize*2.0;  // factor by how steps per voxel sidelength (maybe?)

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
    float aspect = float(uResolution.x)/float(uResolution.y);
    vec2 uv = gl_FragCoord.xy/uResolution;
    uv.y = 1.0-uv.y;

    // DEBUGTEST: manually init lights
    gLightCol[0] = vec3(1.0, 0.9, 0.8);
    gLightPos[0] = vec3(0.0, 2.0, 0.0);
    gLightPos[0].x = 2.0*sin(uTime);
    gLightPos[0].z = 2.0*cos(uTime);


    //-----------------------------------------------------
    // CAMERA RAY
    //-----------------------------------------------------
    
    // camera ray
    vec3 C = normalize(uCamLookAt-uCamPosition);

    // calc A (screen x)
    // calc B (screen y) then scale down relative to aspect
    // fov is for screen x axis
    vec3 A = normalize(cross(C,uCamUp));
    vec3 B = -1.0/(aspect)*normalize(cross(A,C));

    // scale by FOV
    float tanFOV = tan(uFOV/180.0*PI);

    vec3 ro = uCamPosition+C 
        + (2.0*uv.x-1.0)*tanFOV*A 
        + (2.0*uv.y-1.0)*tanFOV*B;
    vec3 rd = normalize(ro-uCamPosition);

    
    //-----------------------------------------------------
    // SETUP GLOBAL VARS
    //-----------------------------------------------------

    // size of one texel in normalized texture coords
    gVoxelSize = 1.0/uVoxelRes;
    
    // size of pixel at dist d=1.0
    gPixSizeAtDist = tanFOV / (uResolution.x/2.0);

    // distance corresponding to highest mip level
    gMaxMipDist = 1.0 / gPixSizeAtDist;
    
    // find max mipmap level, starting at 0.0
    gMaxMipLevel = 0.0;
    int tempSize = int(uVoxelRes);
    while (tempSize>1) {
        gMaxMipLevel++;
        tempSize >>= 1;
    }
    
    
    //-----------------------------------------------------
    // DO CONE TRACE
    //-----------------------------------------------------

    // output color
    vec4 cout;

    // calc entry point
    float t;
    if (textureVolumeIntersect(ro, rd, t))
        cout = conetraceSimple(ro+rd*(t+EPS), rd);
    else
        cout = vec4(0.0);

    // pre-multiply alpha to show
    cout.rgb = cout.rgb*cout.a;
    fragColor = cout;
}