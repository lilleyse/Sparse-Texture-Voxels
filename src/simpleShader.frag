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
    uvec2 uResolution;
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

const uint MAX_STEPS = 128;
const float ALPHA_THRESHOLD = 0.95;
float gStepSize;

//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

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

// simple alpha blending
vec4 raymarchSimple(vec3 ro, vec3 rd) {
  vec3 step = rd*gStepSize;
  vec3 pos = ro;
  
  vec4 color = vec4(0.0);
  
  for (int i=0; i<MAX_STEPS; ++i) {
    vec4 src = texture(testTexture, pos);
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

void main()
{
    float aspect = float(uResolution.x)/float(uResolution.y);
    vec2 uv = gl_FragCoord.xy/uResolution;
    uv.y = 1.0-uv.y;
    
    //vec3 CAMLOOK = uCamLookAt;
    //vec3 CAMPOS = uCamPosition;
    //vec3 CAMUP = uCamUp;
        
    vec3 CAMLOOK = vec3(0.5);
    vec3 CAMPOS = uCamPosition;//vec3(2.0, 2.0, 4.0);
    vec3 CAMUP = uCamUp;//vec3(0.0, 1.0, 0.0);

    /* CAMERA RAY */
    vec3 C = normalize(CAMLOOK-CAMPOS);
    vec3 A = normalize(cross(C,CAMUP));
    vec3 B = -1.0/(aspect)*normalize(cross(A,C));
    
    // scale A and B by root3/3 : fov = 30 degrees
    vec3 ro = CAMPOS+C + (2.0*uv.x-1.0)/ROOTTHREE*A + (2.0*uv.y-1.0)/ROOTTHREE*B;
    vec3 rd = normalize(ro-CAMPOS);

    // output color
    vec4 cout;

    // calc entry point
    {
        float t;
        if (cubeIntersect(vec3(0.0), vec3(1.0), ro, rd, t)) {
            // step_size = root_three / max_steps ; to get through diagonal
            gStepSize = ROOTTHREE / float(MAX_STEPS);

            cout = raymarchSimple(ro+rd*(t+EPS), rd);
        }
        else {
            cout = vec4(0.0);
        }
    }

    // test, sample the 3D texture
    //uv.x *= aspect;  // correct to square
    //vec3 textureIndex = vec3(uv, fract(uTime));
    //vec4 color = texture(testTexture, textureIndex);

    // pre-multiply alpha to show
    cout.rgb = cout.rgb*cout.a;
    fragColor = cout;
}