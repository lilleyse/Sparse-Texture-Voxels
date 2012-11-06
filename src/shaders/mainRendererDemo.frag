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
#define COLOR_TEXTURE_POSX_3D_BINDING            1 // right direction
#define COLOR_TEXTURE_NEGX_3D_BINDING            2 // left direction
#define COLOR_TEXTURE_POSY_3D_BINDING            3 // up direction
#define COLOR_TEXTURE_NEGY_3D_BINDING            4 // down direction
#define COLOR_TEXTURE_POSZ_3D_BINDING            5 // front direction
#define COLOR_TEXTURE_NEGZ_3D_BINDING            6 // back direction
#define SHADOW_MAP_BINDING                       7
#define DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING    8

// Image binding points
#define COLOR_IMAGE_POSX_3D_BINDING              0 // right direction
#define COLOR_IMAGE_NEGX_3D_BINDING              1 // left direction
#define COLOR_IMAGE_POSY_3D_BINDING              2 // up direction
#define COLOR_IMAGE_NEGY_3D_BINDING              3 // down direction
#define COLOR_IMAGE_POSZ_3D_BINDING              4 // front direction
#define COLOR_IMAGE_NEGZ_3D_BINDING              5 // back direction

// Shadow Map FBO
#define SHADOW_MAP_FBO_BINDING     0
#define BLURRED_MAP_FBO_BINDING    1

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
    mat4 uWorldToShadowMap;
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
    int uCurrentMipLevel;
};


//---------------------------------------------------------
// TRIANGLE ENGINE
//---------------------------------------------------------

layout(binding = DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING) uniform sampler2DArray diffuseTextures[MAX_TEXTURE_ARRAYS];

in block
{
    vec3 position;
    vec3 normal;
    vec4 shadowMapPos;
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

layout(binding = SHADOW_MAP_BINDING) uniform sampler2D shadowMap;  
layout(binding = COLOR_TEXTURE_POSX_3D_BINDING) uniform sampler3D tVoxColor;
layout(binding = COLOR_TEXTURE_NEGX_3D_BINDING) uniform sampler3D tVoxColorNegX;
layout(binding = COLOR_TEXTURE_POSY_3D_BINDING) uniform sampler3D tVoxColorPosY;
layout(binding = COLOR_TEXTURE_NEGY_3D_BINDING) uniform sampler3D tVoxColorNegY;
layout(binding = COLOR_TEXTURE_POSZ_3D_BINDING) uniform sampler3D tVoxColorPosZ;
layout(binding = COLOR_TEXTURE_NEGZ_3D_BINDING) uniform sampler3D tVoxColorNegZ;


#define STEPSIZE_WRT_TEXEL 0.3333  // Cyril uses 1/3
#define TRANSMIT_MIN 0.05
#define TRANSMIT_K  8.0
#define INDIR_DIST_K 4.0
#define INDIR_K 1.5
#define AO_DIST_K 0.3
#define JITTER_K 0.025

vec3 gNormal, gDiffuse;
float gTexelSize, gRandVal;

//---------------------------------------------------------
// UTILITIES
//---------------------------------------------------------

// http://www.ozone3d.net/blogs/lab/20110427/glsl-random-generator/
float rand(vec2 n) {
    return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

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

vec4 sampleAnisotropic(vec3 pos, vec3 dir, float mipLevel) {
    vec4 xtexel = dir.x > 0.0 ? 
        textureLod(tVoxColorNegX, pos, mipLevel) : 
        textureLod(tVoxColor, pos, mipLevel);

    vec4 ytexel = dir.y > 0.0 ? 
        textureLod(tVoxColorNegY, pos, mipLevel) : 
        textureLod(tVoxColorPosY, pos, mipLevel);

    vec4 ztexel = dir.z > 0.0 ? 
        textureLod(tVoxColorNegZ, pos, mipLevel) : 
        textureLod(tVoxColorPosZ, pos, mipLevel);

    // get scaling factors for each axis
    dir = abs(dir);

    // TODO: correctly weight averaged output color
    return (dir.x*xtexel + dir.y*ytexel + dir.z*ztexel);
}

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

        float vocc = textureLod(tVoxColor, pos, mipLevel).a;
        if(vocc > 0.0) {
            float dtm = exp( -TRANSMIT_K * STEPSIZE_WRT_TEXEL * vocc );
            tm *= dtm;
            col += (1.0-dtm) * sampleAnisotropic(pos, rd, mipLevel).rgb;
        }
          
        // increment
        float stepSize = pixSize * STEPSIZE_WRT_TEXEL;
        //stepSize += gRandVal*pixSize*JITTER_K;
        dist += stepSize;
        pos += stepSize*rd;
    }
    
    return col;
}

vec4 conetraceIndir(vec3 ro, vec3 rd, float fov) {
    vec3 pos = ro;
    float dist = 0.0;
    float pixSizeAtDist = tan(fov);

    vec4 col = vec4(0.0);   // accumulated color
    float tm = 1.0;         // accumulated transmittance

    while(tm > TRANSMIT_MIN &&
        pos.x < 1.0 && pos.x > 0.0 &&
        pos.y < 1.0 && pos.y > 0.0 &&
        pos.z < 1.0 && pos.z > 0.0) {

        // calc mip size, clamp min to texelsize
        float pixSize = max(dist*pixSizeAtDist, gTexelSize);
        float mipLevel = max(log2(pixSize/gTexelSize), 0.0);

        float vocc = textureLod(tVoxColor, pos, mipLevel).a;
        if(vocc > 0.0) {
            float dtm = exp( -TRANSMIT_K * STEPSIZE_WRT_TEXEL * vocc );
            tm *= dtm;

            // calc local illumination
            vec3 lightCol = (1.0-dtm) * sampleAnisotropic(pos, rd, mipLevel).rgb;
            vec3 localColor = gDiffuse*lightCol;
            localColor *= (INDIR_DIST_K*dist)*(INDIR_DIST_K*dist);
            col.rgb += localColor;
            // gDiffuse can be factored out, but here for now for clarity
        }
    
        // increment
        float stepSize = pixSize * STEPSIZE_WRT_TEXEL;
        stepSize += gRandVal*pixSize*JITTER_K;
        dist += stepSize;
        pos += stepSize*rd;
    }
  
    // weight AO by distance f(r) = 1/(1+K*r)
    float visibility = min( tm*(1.0+AO_DIST_K*dist), 1.0);

    return vec4(INDIR_K*col.rgb, visibility);
}

float getVisibility()
{
    vec4 fragLightPos = vertexData.shadowMapPos / vertexData.shadowMapPos.w;
    float fragLightDepth = fragLightPos.z;
    vec2 moments = texture(shadowMap, fragLightPos.xy).rg;
        
    // Surface is fully lit.
    if (fragLightDepth <= moments.x)
        return 1.0;
    
    // How likely this pixel is to be lit (p_max)
    float variance = moments.y - (moments.x*moments.x);
    variance = max(variance,0.00002);
    
    float d = moments.x - fragLightDepth;
    float p_max = variance / (variance + d*d);
    return p_max;
}

void main()
{
    // current vertex info
    vec3 pos = vertexData.position;
    vec3 cout = vec3(0.0);
    gNormal = normalize(vertexData.normal);
    gDiffuse = getDiffuseColor(getMeshMaterial()).rgb;
    
    // calc globals
    gRandVal = 0.0;//rand(pos.xy);
    gTexelSize = 1.0/uTextureRes; // size of one texel in normalized texture coords
    float voxelOffset = gTexelSize*2.0;

    #define PASS_DIFFUSE
    #define PASS_INDIR
    #define PASS_SPEC

    #ifdef PASS_INDIR
    vec4 indir = vec4(0.0);
    {
        #define NUM_DIRS 4.0
        const float FOV = radians(45.0);
        const float NORMAL_ROTATE = radians(45.0);
        const float ANGLE_ROTATE = 2.0*PI / NUM_DIRS;

        vec3 axis = findPerpendicular(gNormal);
        for (float i=0.0; i<NUM_DIRS; i++) {
            vec3 rotatedAxis = rotate(axis, ANGLE_ROTATE*(i+EPS), gNormal);
            vec3 rd = rotate(gNormal, NORMAL_ROTATE, rotatedAxis);
            indir += conetraceIndir(pos+rd*voxelOffset, rd, FOV);
        }

        indir /= NUM_DIRS;

        #undef NUM_DIRS
    }
    #endif

    #ifdef PASS_SPEC
    vec3 spec = vec3(0.0);
    {    
        // single cone in reflected eye direction
        const float FOV = radians(uSpecularFOV);
        vec3 rd = normalize(pos-uCamPos);
        rd = reflect(rd, gNormal);
        spec = conetraceSpec(pos+rd*voxelOffset, rd, FOV);
    }
    #endif

    #ifdef PASS_DIFFUSE
    float visibility = getVisibility();
    float LdotN = max(dot(uLightDir, gNormal), 0.0);
    vec4 diffuse = getDiffuseColor(getMeshMaterial());
    cout += diffuse.rgb * uLightColor * LdotN * visibility;
    #endif
    #ifdef PASS_INDIR
    cout += indir.rgb;
    cout *= indir.a;
    #endif
    #ifdef PASS_SPEC
    cout = mix(cout, spec, uSpecularAmount);
    #endif

    // adjust blown out colors
    float difference = max(0.0,max(cout.r - 1.0, max(cout.g - 1.0, cout.b - 1.0)));
    cout = clamp(cout - difference, 0.0, 1.0);

    fragColor = vec4(cout.rgb, 1.0);
}