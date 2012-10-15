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
#define NORMAL_IMAGE_3D_BINDING                  1
#define COLOR_IMAGE_3D_BINDING                   2         

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
#define MAX_3D_MIPMAP_LEVELS            10

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

layout(location = 0, index = 0) out vec4 fragColor;
layout(binding = COLOR_TEXTURE_3D_BINDING) uniform sampler3D colorTexture;
layout(binding = NORMAL_TEXTURE_3D_BINDING) uniform sampler3D normalTexture;

in vec2 vUV;
uniform float uTextureRes;

const int MAX_STEPS = 256;
const float STEPSIZE_WRT_TEXEL = 0.3333;  // Cyrill uses 1/3
const float ALPHA_THRESHOLD = 0.95;
const float TRANSMIT_MIN = 0.05;
const float TRANSMIT_K = 1.0;

float gTexelSize;
float gPixSizeAtDist;


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
vec4 conetraceSimple(vec3 ro, vec3 rd) {
  vec3 pos = ro;
  
  vec4 color = vec4(0.0);
  
  for (int i=0; i<MAX_STEPS; ++i) {
    float dist = distance(pos, uCamPos);

    // size of texel cube we want to be looking into
    // correctly interpolated texel size, automatic
    float pixSize = gPixSizeAtDist * dist;
    
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
    vec4 src = textureLod(colorTexture, pos, mipLevel);
    
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

// transmittance accumulation
vec4 conetraceAccum(vec3 ro, vec3 rd) {
  vec3 pos = ro;
  
  vec3 col = vec3(0.0);   // accumulated color
  float tm = 1.0;         // accumulated transmittance
  
  for (int i=0; i<MAX_STEPS; ++i) {
    float dist = distance(pos, uCamPos);

    // size of texel cube we want to be looking into
    // correctly interpolated texel size, automatic
    float pixSize = gPixSizeAtDist * dist;
    
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
    vec4 texel = textureLod(colorTexture, pos, mipLevel);
    
    // alpha normalized to 1 texel, i.e., 1.0 alpha is 1 solid block of texel
    // no need weight by "stepSize" since "pixSize" is size of an imaginary 
    // texel cube exactly the size of a mip cube we want, if it existed, 
    // but it doesn't so we interpolate between two mips to approximate it
    // but need to weight by stepsize within texel

    // delta transmittance
    float dtm = exp( -TRANSMIT_K * STEPSIZE_WRT_TEXEL*texel.a );
    tm *= dtm;

    col += (1.0-dtm)*texel.rgb*tm;

    pos += stepSize*rd;
    
    if (tm < TRANSMIT_MIN ||
      pos.x > 1.0 || pos.x < 0.0 ||
      pos.y > 1.0 || pos.y < 0.0 ||
      pos.z > 1.0 || pos.z < 0.0)
      break;
  }
  
  float alpha = 1.0-tm;
  return vec4( alpha==0 ? col : col/alpha , alpha);;
}

void main()
{
	// flip uv.y
	vec2 uv = vec2(vUV.x, 1.0-vUV.y);

    //-----------------------------------------------------
    // CAMERA RAY
    //-----------------------------------------------------

    // camera ray
    vec3 C = normalize(uCamLookAt-uCamPos);

    // calc A (screen x)
    // calc B (screen y) then scale down relative to aspect
    // fov is for screen x axis
    vec3 A = normalize(cross(C,uCamUp));
    vec3 B = -1.0/(uAspect)*normalize(cross(A,C));

    // scale by FOV
    float tanFOV = tan(radians(uFOV));

    vec3 rd = normalize(
        C + (2.0*uv.x-1.0)*tanFOV*A + (2.0*uv.y-1.0)*tanFOV*B
    );

    
    //-----------------------------------------------------
    // SETUP GLOBAL VARS
    //-----------------------------------------------------

    // size of one texel in normalized texture coords
    gTexelSize = 1.0/uTextureRes;
    
    // size of pixel at dist d=1.0
    gPixSizeAtDist = tanFOV / (uResolution.x/2.0);

    
    //-----------------------------------------------------
    // DO CONE TRACE
    //-----------------------------------------------------

    // output color
    vec4 cout;

    // calc entry point
    float t;
    if (textureVolumeIntersect(uCamPos+rd*EPS, rd, t))
        cout = conetraceAccum(uCamPos+rd*(t+EPS), rd);
    else
        cout = vec4(0.0);

    // background color
    vec4 bg = vec4(vec3(0.0, 0.0, (1.0-vUV.y)/2.0), 1.0);

    // alpha blend cout over bg
    bg.rgb = mix(bg.rgb, cout.rgb, cout.a);
    fragColor = bg;
}