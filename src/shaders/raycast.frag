//---------------------------------------------------------
// GLOBALS
//---------------------------------------------------------

#version 420 core

// Vertex attribute indexes
#define POSITION_ATTR            0
#define NORMAL_ATTR              1
#define UV_ATTR                  2
#define PROPERTY_INDEX_ATTR      3
#define DEBUG_TRANSFORM_ATTR     4
#define DEBUG_COLOR_ATTR         5

// Uniform buffer objects binding points
#define PER_FRAME_UBO_BINDING            0
#define LIGHT_UBO_BINDING                1
#define MESH_MATERIAL_ARRAY_BINDING      2
#define POSITION_ARRAY_BINDING           3

// Sampler binding points
#define NON_USED_TEXTURE                         0
#define COLOR_TEXTURE_3D_BINDING                 1
#define NORMAL_TEXTURE_3D_BINDING                2
#define DEFERRED_POSITIONS_TEXTURE_BINDING       3
#define DEFERRED_COLORS_TEXTURE_BINDING          4
#define DEFERRED_NORMALS_TEXTURE_BINDING         5
#define DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING    6       

// Image binding points
#define NON_USED_IMAGE                           0
#define COLOR_IMAGE_3D_BINDING                   1
#define NORMAL_IMAGE_3D_BINDING                  2           

// Framebuffer object outputs
#define DEFERRED_POSITIONS_FBO_BINDING       0
#define DEFERRED_COLORS_FBO_BINDING          1
#define DEFERRED_NORMALS_FBO_BINDING         2

// Object properties
#define POSITION_INDEX        0
#define MATERIAL_INDEX        1

// Max values
#define MAX_TEXTURE_ARRAYS              10
#define FBO_BINDING_POINT_ARRAY_SIZE    4
#define NUM_OBJECTS_MAX                 500
#define NUM_MESHES_MAX                  500
#define MAX_POINT_LIGHTS                8


layout(std140, binding = PER_FRAME_UBO_BINDING) uniform PerFrameUBO
{
    mat4 uViewProjection;
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

#define TEXTURE_TYPE colorTexture

layout (location = 0, index = 0) out vec4 fragColor;
layout (binding = COLOR_TEXTURE_3D_BINDING) uniform sampler3D colorTexture;
layout (binding = NORMAL_TEXTURE_3D_BINDING) uniform sampler3D normalTexture;

in vec2 vUV;
uniform float uMipLevel;

const uint MAX_STEPS = 64;
const float ALPHA_THRESHOLD = 0.95;
const float TRANSMIT_MIN = 0.05;
const float TRANSMIT_K = 8.0;

float gStepSize;

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

// special case, for optimization
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
vec4 raymarchSimple(vec3 ro, vec3 rd) {
  vec3 step = rd*gStepSize;
  vec3 pos = ro;
  
  vec4 color = vec4(0.0);
  
  for (int i=0; i<MAX_STEPS; ++i) {

    vec4 src = textureLod(TEXTURE_TYPE, pos, uMipLevel);
    src.a *= gStepSize;  // factor by how steps per voxel diag


    // alpha blending
    vec4 dst = color;
    color.a = src.a + dst.a*(1.0-src.a);
    color.rgb = EQUALSZERO(color.a) ? vec3(0.0) : 
        (src.rgb*src.a + dst.rgb*dst.a*(1.0-src.a)) / color.a;

    pos += step;
    
    if (color.a > ALPHA_THRESHOLD ||
      pos.x > 1.0 || pos.x < 0.0 ||
      pos.y > 1.0 || pos.y < 0.0 ||
      pos.z > 1.0 || pos.z < 0.0)
      break;
  }
  
  return color;
}

// raymarch to get transmittance
float getTransmittance(vec3 ro, vec3 rd) {
  vec3 step = rd*gStepSize;
  vec3 pos = ro;
  
  float tm = 1.0;
  
  for (int i=0; i<MAX_STEPS; ++i) {
    tm *= exp( -TRANSMIT_K*gStepSize*textureLod(TEXTURE_TYPE, pos, uMipLevel).a );

    pos += step;
    
    if (tm < TRANSMIT_MIN ||
      pos.x > 1.0 || pos.x < 0.0 ||
      pos.y > 1.0 || pos.y < 0.0 ||
      pos.z > 1.0 || pos.z < 0.0)
      break;
  }
  
  return tm;
}

// raymarch transmittance from r0 to r1
float getTransmittanceToDst(vec3 r0, vec3 r1) {
  vec3 dir = normalize(r1-r0);
  vec3 step = dir*gStepSize;
  vec3 pos = r0;
  
  float tm = 1.0;
  
  for (int i=0; i<MAX_STEPS; ++i) {
    tm *= exp( -TRANSMIT_K*gStepSize*textureLod(TEXTURE_TYPE, pos, uMipLevel).a );

    pos += step;

    // check if pos passed r1
    if ( dot((r1-pos),dir) < 0.0 )
        break;
  }
  
  return tm;
}

// raymarch with light transmittance
vec4 raymarchLight(vec3 ro, vec3 rd) {
  vec3 step = rd*gStepSize;
  vec3 pos = ro;
    
  vec3 col = vec3(0.0);   // accumulated color
  float tm = 1.0;         // accumulated transmittance
  
  for (int i=0; i<MAX_STEPS; ++i) {
    vec4 texel = textureLod(TEXTURE_TYPE, pos, uMipLevel);

    // delta transmittance
    float dtm = exp( -TRANSMIT_K*gStepSize*texel.a );
    tm *= dtm;
    
    // get contribution per light
    for (int k=0; k<LIGHT_NUM; ++k) {
      vec3 lo = gLightPos[k];
      vec3 ld = normalize(pos-lo);
      float t;
      textureVolumeIntersect(lo, ld, t);
      float ltm = getTransmittanceToDst(lo+ld*(t+EPS),pos);
      
      col += (1.0-dtm) * texel.rgb*gLightCol[k] * tm * ltm;
    }
    
    pos += step;
    
    if (tm < TRANSMIT_MIN ||
      pos.x > 1.0 || pos.x < 0.0 ||
      pos.y > 1.0 || pos.y < 0.0 ||
      pos.z > 1.0 || pos.z < 0.0)
      break;
  }
  
  float alpha = 1.0-tm;
  return vec4( alpha==0 ? col : col/alpha , alpha);
}

void main()
{
    // DEBUGTEST: manually init lights
    gLightCol[0] = vec3(1.0, 0.9, 0.8);
    gLightPos[0] = vec3(0.0, 2.0, 0.0);
    gLightPos[0].x = 2.0*sin(uTime);
    gLightPos[0].z = 2.0*cos(uTime);

	// flip uv.y
	vec2 uv = vec2(vUV.x, 1.0-vUV.y);

    // camera ray
    vec3 C = normalize(uCamLookAt-uCamPos);

    // calc A (screen x)
    // calc B (screen y) then scale down relative to aspect
    // fov is for screen x axis
    vec3 A = normalize(cross(C,uCamUp));
    vec3 B = -1.0/(uAspect)*normalize(cross(A,C));

    // scale by FOV
    float tanFOV = tan(radians(uFOV));

    vec3 ro = uCamPos+C
        + (2.0*uv.x-1.0)*tanFOV*A 
        + (2.0*uv.y-1.0)*tanFOV*B;
    vec3 rd = normalize(ro-uCamPos);

    // output color
    vec4 cout;

    // calc entry point
    float t;
    if (textureVolumeIntersect(uCamPos, rd, t)) {
        // step_size = root_three / max_steps ; to get through diagonal
        gStepSize = ROOTTHREE / float(MAX_STEPS);

        cout = raymarchLight(uCamPos+rd*(t+EPS), rd);
    }
    else {
        cout = vec4(0.0);
    }

    // background color
    vec4 bg = vec4(vec3(0.0, 0.0, (1.0-vUV.y)/2.0), 1.0);

    // alpha blend cout over bg
    bg.rgb = mix(bg.rgb, cout.rgb, cout.a);
    fragColor = bg;
}