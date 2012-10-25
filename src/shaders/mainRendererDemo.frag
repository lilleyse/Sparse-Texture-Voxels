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
#define COLOR_TEXTURE_3D_BINDING                 1
#define NORMAL_TEXTURE_3D_BINDING                2
#define DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING    3

// Image binding points
#define COLOR_IMAGE_3D_BINDING_BASE              0
#define COLOR_IMAGE_3D_BINDING_CURR              1
#define COLOR_IMAGE_3D_BINDING_NEXT              2
#define NORMAL_IMAGE_3D_BINDING                  3

// Object properties
#define POSITION_INDEX        0
#define MATERIAL_INDEX        1

// Max values
#define MAX_TEXTURE_ARRAYS               10
#define NUM_OBJECTS_MAX                  500
#define NUM_MESHES_MAX                   500
#define MAX_POINT_LIGHTS                 8

layout(std140, binding = PER_FRAME_UBO_BINDING) uniform PerFrameUBO
{
    mat4 uViewProjection;
    vec3 uCamLookAt;
    vec3 uCamPos;
    vec3 uCamUp;
    vec3 uLightDir;
    vec3 uLightColor;
    vec2 uResolution;
    float uAspect;
    float uTime;
    float uTimestamp;
    float uFOV;
    float uTextureRes;
    float uNumMips;
    float uSpecularFOV;
    float uSpecularAmount;
};


//---------------------------------------------------------
// TRIANGLE ENGINE
//---------------------------------------------------------

layout(binding = DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING) uniform sampler2DArray diffuseTextures[MAX_TEXTURE_ARRAYS];

in block
{
    vec3 position;
    vec3 normal;
    vec2 uv;
    flat ivec2 propertyIndex;
} vertexData;

struct MeshMaterial
{
    vec4 diffuseColor;
    vec4 specularColor;
    ivec2 textureLayer;
};

layout(std140, binding = MESH_MATERIAL_ARRAY_BINDING) uniform MeshMaterialArray
{
    MeshMaterial meshMaterialArray[NUM_MESHES_MAX];
};

MeshMaterial getMeshMaterial()
{
    int index = vertexData.propertyIndex[MATERIAL_INDEX];
    return meshMaterialArray[index];
}

vec4 getDiffuseColor(MeshMaterial material)
{
    int textureId = material.textureLayer.x;
    int textureLayer = material.textureLayer.y;
    return textureId == -1 ? 
        material.diffuseColor : 
        texture(diffuseTextures[textureId], vec3(vertexData.uv, textureLayer));
}


//---------------------------------------------------------
// SHADER CONSTANTS
//---------------------------------------------------------

#define EPS       0.0001
#define EPS2      0.05
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

layout(binding = COLOR_TEXTURE_3D_BINDING) uniform sampler3D tVoxColor;
layout(binding = NORMAL_TEXTURE_3D_BINDING) uniform sampler3D tVoxNormal;

#define STEPSIZE_WRT_TEXEL 0.3333  // Cyril uses 1/3
#define TRANSMIT_MIN 0.05
#define TRANSMIT_K 3.0
#define INDIR_DIST_K 5.0
#define INDIR_K 1.5

vec3 gNormal, gDiffuse;
float gTexelSize = 0.0;

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
    //vec3 result;
    //if (EQUALS(abs(v.x),1.0) || EQUALS(abs(v.y),1.0))
        //result = vec3(0.0, 0.0, 1.0);
    //else if (EQUALS(abs(v.z),1.0))
        //result = vec3(1.0, 0.0, 0.0);
    //else
        //result = normalize(vec3(1.0, 0.0, -v.x/(v.z+EPS8)));
    //return result;

    // fast dirty method
    return normalize( vec3(1.0, 0.0, -v.x/(v.z+EPS8)) );
}


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

vec3 conetraceSpec(vec3 ro, vec3 rd, float fov) {
  vec3 pos = ro;
  float dist = 0.0;
  float pixSizeAtDist = tan(fov);

  vec3 col = vec3(0.0);   // accumulated color
  float tm = 1.0;         // accumulated transmittance

  while(tm > TRANSMIT_MIN &&
        pos.x < 1.0 && pos.x > 0.0 &&
        pos.y < 1.0 && pos.y > 0.0 &&
        pos.z < 1.0 && pos.z > 0.0) {

    // calc mip size, clamp min to texelsize
    float pixSize = max(dist*pixSizeAtDist, gTexelSize);
    float mipLevel = max(log2(pixSize/gTexelSize), 0.0);

    vec4 vcol = textureLod(tVoxColor, pos, mipLevel);
    float dtm = exp( -TRANSMIT_K * STEPSIZE_WRT_TEXEL * vcol.a );
    tm *= dtm;

    vec4 vnor = textureLod(tVoxNormal, pos, 0.0);//mipLevel);

    // render
    vec3 difflight = (1.0-dtm) * vcol.rgb;  // diffuseCol*lightCol
    vec3 reflectedDir = vnor.xyz;
    float LdotN = abs(vnor.w);
    
    #define KD 0.6
    #define KS 0.4
    #define SPEC 5
    col += KD*difflight*LdotN + KS*pow(max(dot(reflectedDir,-rd), 0.0), SPEC);
    
    // increment
    float stepSize = pixSize * STEPSIZE_WRT_TEXEL;
    dist += stepSize;
    pos += stepSize*rd;
  }

  return col;
}

vec3 conetraceIndir(vec3 ro, vec3 rd, float fov) {
  vec3 pos = ro;
  float dist = 0.0;
  float pixSizeAtDist = tan(fov);

  vec3 col = vec3(0.0);   // accumulated color
  float tm = 1.0;         // accumulated transmittance

  while(tm > TRANSMIT_MIN &&
        pos.x < 1.0 && pos.x > 0.0 &&
        pos.y < 1.0 && pos.y > 0.0 &&
        pos.z < 1.0 && pos.z > 0.0) {

    // calc mip size, clamp min to texelsize
    float pixSize = max(dist*pixSizeAtDist, gTexelSize);
    float mipLevel = max(log2(pixSize/gTexelSize), 0.0);

    vec4 vcol = textureLod(tVoxColor, pos, mipLevel);
    float dtm = exp( -TRANSMIT_K * STEPSIZE_WRT_TEXEL * vcol.a );
    tm *= dtm;

    // calc local illumination
    vec3 lightCol = (1.0-dtm) * vcol.rgb;    
    //vec3 lightDir = textureLod(tVoxNormal, pos, mipLevel).xyz;
    vec3 localColor = gDiffuse*lightCol;//*dot(lightDir, gNormal); 
    localColor *= (INDIR_DIST_K*dist)*(INDIR_DIST_K*dist);
    col += localColor;
    // gDiffuse can be factored out, but here for now for clarity
    
    // increment
    float stepSize = pixSize * STEPSIZE_WRT_TEXEL;
    dist += stepSize;
    pos += stepSize*rd;
  }

  return INDIR_K*col;
}

void main()
{
    // current vertex info
    vec3 pos = vertexData.position;
    gNormal = normalize(vertexData.normal);    
    gDiffuse = getDiffuseColor(getMeshMaterial()).rgb;

    vec4 radiance = textureLod(tVoxColor, pos, 1.0);
    float LdotN = abs(textureLod(tVoxNormal, pos, 0.0).w);
    
    // calc globals
    gTexelSize = 1.0/uTextureRes; // size of one texel in normalized texture coords
    float voxelOffset = gTexelSize;//*ROOTTHREE;
    
    //#define PASS_DIFFUSE
    #define PASS_INDIR
    //#define PASS_SPEC

    #ifdef PASS_INDIR
    vec3 indir = vec3(0.0);
    {
        #define NUM_DIRS 6.0
        #define NUM_RADIAL_DIRS 5.0
        const float FOV = radians(30.0);
        const float NORMAL_ROTATE = radians(60.0-5.0);  // should be 60.0
        const float ANGLE_ROTATE = 2.0*PI / NUM_RADIAL_DIRS;

        vec3 axis = findPerpendicular(gNormal);
        for (float i=0.0; i<NUM_RADIAL_DIRS; i++) {
            vec3 rotatedAxis = rotate(axis, ANGLE_ROTATE*(i+EPS), gNormal);
            vec3 rd = rotate(gNormal, NORMAL_ROTATE, rotatedAxis);
            indir += conetraceIndir(pos+rd*voxelOffset, rd, FOV);
        }

        indir += conetraceIndir(pos+gNormal*voxelOffset, gNormal, FOV);
        indir /= NUM_DIRS;

        #undef NUM_DIRS
        #undef NUM_RADIAL_DIRS
    }
    #endif

    #ifdef PASS_SPEC
    vec3 spec = vec3(0.0);
    {    
        // single cone in reflected eye direction
        const float FOV = radians(uSpecularFOV);
        vec3 rd = normalize(pos-uCamPos);
        rd = reflect(rd, gNormal);
        spec = conetraceSpec(pos+rd*voxelOffset*2.0, rd, FOV);
    }
    #endif

    vec3 cout = vec3(0.0);
    #ifdef PASS_DIFFUSE
    cout += radiance.rgb * LdotN;
    #endif
    #ifdef PASS_INDIR
    cout += indir;
    #endif
    #ifdef PASS_SPEC
    cout = mix(cout, spec, uSpecularAmount);
    #endif
    fragColor = vec4(cout, 1.0);
}