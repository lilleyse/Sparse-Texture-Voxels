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
#define ROOTTWO   1.41421356
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
const float STEPSIZE_WRT_TEXEL = 0.3333;  // Cyrill uses 1/3
const float TRANSMIT_MIN = 0.05;
const float TRANSMIT_K = 1.0;
const float AO_DIST_K = 0.5;

float gTexelSize;


//---------------------------------------------------------
// UTILITIES
//---------------------------------------------------------

// rotate vector a given angle over a given axis
// source: http://www.groupsrv.com/computers/about180175.html
vec3 rotate(vec3 vector, float angle, vec3 axis) {
    float satangle = radians(angle); 
    float csat = cos(satangle); 
    float ssat = sin(satangle); 
    float usat = 1.0 - csat;

    mat3 rotmat;
    rotmat[0][0] = axis.x*axis.x*usat + csat;	            //	Mat[0] = (x * x * u) + c; 
    rotmat[1][0] = axis.y*axis.x*usat - (axis.z*ssat);      //	Mat[4] = (y * x * u) - (z * s); 
    rotmat[2][0] = axis.z*axis.x*usat + (axis.y*ssat);      //	Mat[8] = (z * x * u) + (y * s); 

    rotmat[0][1] = axis.x*axis.y*usat + (axis.z*ssat);      //	Mat[1] = (x * y * u) + (z * s); 
    rotmat[1][1] = axis.y*axis.y*usat + csat;	            //	Mat[5] = (y * y * u) + c; 
    rotmat[2][1] = axis.z*axis.y*usat - (axis.x*ssat);      //	Mat[9] = (z * y * u) - (x * s); 

    rotmat[0][2] = axis.x*axis.z*usat - (axis.y*ssat);      //	Mat[2] = (x * z * u) - (y * s); 
    rotmat[1][2] = axis.x*axis.z*usat - (axis.y*ssat);      //	Mat[6] = (y * z * u) + (x * s); 
    rotmat[2][2] = axis.z*axis.z*usat + csat;	            //	Mat[10] = (z * z * u) + c; 

    return vector * rotmat; 
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


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

// transmittance accumulation
vec4 conetraceAccum(vec3 ro, vec3 rd, float fov) {
  vec3 pos = ro;  
  float dist = 0.0;
  float pixSizeAtDist = tan(fov);
  
  vec3 col = vec3(0.0);   // accumulated color
  float tm = 1.0;         // accumulated transmittance
  
  for (int i=0; i<MAX_STEPS; ++i) {
    // size of texel cube we want
    float pixSize = dist * pixSizeAtDist;
    
    // calc mip size, clamp min to texelsize
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
    vec4 texel = textureLod(tVoxColor, pos, mipLevel);
    
    // alpha normalized to 1 texel, i.e., 1.0 alpha is 1 solid block of texel
    // delta transmittance
    float dtm = exp( -TRANSMIT_K * STEPSIZE_WRT_TEXEL*texel.a );
    tm *= dtm;

    col += (1.0-dtm)*texel.rgb*tm;

    // increment
    dist += stepSize;
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


// for AO, dist weighted transmittance accumulation
float conetraceVisibility(vec3 ro, vec3 rd, float fov) {
  vec3 pos = ro;  
  float dist = 0.0;
  float pixSizeAtDist = tan(fov);

  float tm = 1.0;         // accumulated transmittance
  
  for (int i=0; i<MAX_STEPS; ++i) {
    // size of texel cube
    float pixSize =  dist * pixSizeAtDist;
    
    // calc mip size
    float mipLevel;
    if (pixSize > gTexelSize) {
        mipLevel = log2(pixSize/gTexelSize);
    }
    else {
        mipLevel = 0.0;
        pixSize = gTexelSize;
    }

    // take step relative to the interpolated size
    float stepSize = pixSize * STEPSIZE_WRT_TEXEL;

    // sample texture
    vec4 texel = textureLod(tVoxColor, pos, mipLevel);

    // update transmittance
    tm *= exp( -TRANSMIT_K * STEPSIZE_WRT_TEXEL*texel.a );
    
    // increment
    dist += stepSize;
    pos += stepSize*rd;
    
    if (tm < TRANSMIT_MIN ||
      pos.x > 1.0 || pos.x < 0.0 ||
      pos.y > 1.0 || pos.y < 0.0 ||
      pos.z > 1.0 || pos.z < 0.0)
      break;
  }

  // weight by distance f(r) = 1/(1+K*r)
  float weight = (1.0+AO_DIST_K*dist);
  
  return tm * weight;
}

void main()
{
    //-----------------------------------------------------
    // SETUP VARS
    //-----------------------------------------------------

    // size of one texel in normalized texture coords
    gTexelSize = 1.0/uTextureRes;

    vec3 pos = texture(tPosition, vUV).rgb;
    vec3 nor = texture(tNormal, vUV).rgb;
    vec4 col = texture(tColor, vUV);


    //-----------------------------------------------------
    // GATHER AO
    //-----------------------------------------------------
    
    const float fov = radians(45.0);
    const int NUM_AO_DIRS = 1;
    float ao = 0.0;
    for (int i=0; i<NUM_AO_DIRS; i++) {
        vec3 V = nor;
        float t;
        if ( col.a!=0.0 /*&& textureVolumeIntersect(pos+V*EPS, V+0.00000001, t)*/ )
            ao += conetraceVisibility(pos+V*(t+EPS), V, fov);
        else
            ao += 0.0;
    }
    ao /= float(NUM_AO_DIRS);

    // multiply AO into color
    col.rgb *= ao;


    //-----------------------------------------------------
    // RENDER OUT
    //-----------------------------------------------------
        
    vec4 cout = col;

    // background color
    vec4 bg = vec4(vec3(0.0, 0.0, (1.0-vUV.y)/2.0), 1.0);

    // alpha blend cout over bg
    bg.rgb = mix(bg.rgb, cout.rgb, cout.a);
    fragColor = bg;

    //fragColor = vec4(nor, 1.0);
}