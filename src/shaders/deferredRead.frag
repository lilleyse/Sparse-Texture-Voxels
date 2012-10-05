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
#define EPS8      0.00000001
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

#define MAX_STEPS 128
#define STEPSIZE_WRT_TEXEL 0.3333  // Cyrill uses 1/3
#define TRANSMIT_MIN 0.05
#define TRANSMIT_K 1.0
#define AO_DIST_K 0.5

float gTexelSize;


//---------------------------------------------------------
// UTILITIES
//---------------------------------------------------------

// rotate vector a given angle(rads) over a given axis
// source: http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToMatrix/index.htm
vec3 rotate(vec3 vector, float angle, vec3 axis) {
    float c = cos(angle);
    float s = sin(angle);
    float t = 1.0 - c;

    mat3 rot;
    rot[0][0] = c + axis.x*axis.x*t;
    rot[1][1] = c + axis.y*axis.y*t;
    rot[2][2] = c + axis.z*axis.z*t;

    float tmp1 = axis.x*axis.y*t;
    float tmp2 = axis.z*s;
    rot[1][0] = tmp1 + tmp2;
    rot[0][1] = tmp1 - tmp2;
    tmp1 = axis.x*axis.z*t;
    tmp2 = axis.y*s;
    rot[2][0] = tmp1 - tmp2;
    rot[0][2] = tmp1 + tmp2;
    tmp1 = axis.y*axis.z*t;
    tmp2 = axis.x*s;
    rot[2][1] = tmp1 + tmp2;
    rot[1][2] = tmp1 - tmp2;

    return rot*vector;
}

// find a perpendicular vector, non-particular
// in this case always parallel to xz-plane
// v has to be normalized
vec3 findPerpendicular(vec3 v) {
    // solve: result dot v = 0
    // so: X*v.x + Y*v.y + Z*v.z = 0
    // fix y to 0 - parallel to xz-plane
    // arbitrary fix x to 1.0, but if v.x == 1.0, then fix z
    // so: v.x + Z*v.z = 0
    
    // safe method, rely on floating point
    vec3 result;
    if (EQUALS(abs(v.x),1.0) || EQUALS(abs(v.y),1.0)) 
        result = vec3(0.0, 0.0, 1.0);
    else if (EQUALS(abs(v.z),1.0))
        result = vec3(1.0, 0.0, 0.0);
    else
        result = normalize(vec3(1.0, 0.0, -v.x/(v.z+EPS8)));
        
    return result;

    // fast dirty method
    //return normalize( vec3(1.0, 0.0, -v.x/(v.z+EPS8)) );
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
  alpha /= (1.0+AO_DIST_K*dist);
  return vec4( alpha==0 ? col : col/alpha , alpha);
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
    // COMPUTE COLORS
    //-----------------------------------------------------
    
    // if nothing there, don't color
    if ( col.a!=0.0 ) {
        
        /* AMBIENT OCCLUSION */
        float ao = 0.0;
        {
            // setup cones constants   
            #define NUM_AO_DIRS 6.0
            #define NUM_RADIAL_DIRS 5.0
            const float FOV = radians(30.0);
            const float ANGLE_ROTATE = radians(75.0);

            // radial ring of cones
            vec3 axis = findPerpendicular(nor); // find a perpendicular vector
            for (float i=0.0; i<NUM_RADIAL_DIRS; i++) {
                // rotate that vector around normal (to distribute cone around)
                vec3 rotatedAxis = rotate(axis, ANGLE_ROTATE*(i+EPS), nor);
            
                // ray dir is normal rotated an fov over that vector
                vec3 rd = rotate(nor, FOV, rotatedAxis);

                ao += conetraceVisibility(pos+rd*EPS, rd, FOV);
            }

            // single perpendicular cone (straight up)
            ao += conetraceVisibility(pos+nor*EPS, nor, FOV);

            // finally, divide
            ao /= NUM_AO_DIRS;

            #undef NUM_AO_DIRS
            #undef NUM_RADIAL_DIRS
        }

        /* SPECULAR */
        vec4 spec;
        {            
            const float FOV = radians(10.0);
            vec3 rd = normalize(pos-uCamPos);
            rd = reflect(rd, nor);
            spec = conetraceAccum(pos+rd*EPS, rd, FOV);
        }
        
        /* COMP PASSES */        
        //col.rgb = vec3(ao);
        //col.rgb = mix(col.rgb, spec.rgb*spec.a, 0.6);
        col.rgb *= ao;
    }


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