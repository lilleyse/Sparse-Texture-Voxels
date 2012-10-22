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
#define TRANSMIT_K 1.0
#define AO_DIST_K 0.5

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


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

// assumption, current pos is empty (alpha = 0.0)
float getNonEmptyMipLevel(vec3 pos, float mipLevel) {
    float level = ceil(mipLevel);
    float alpha = 0.0;

    while(level < uNumMips && alpha == 0.0) {
        alpha = textureLod(tVoxColor, pos, ++level).a;
    }

    return level;
}

// params: origin and ray within texture space
// intersect outside bound of the enclosing texel of given mip level
float texelIntersect(vec3 ro, vec3 rd, float mipLevel) {
    float texelSize = pow(2, mipLevel) / uTextureRes;

    vec3 texelIdx = ro/texelSize;

    vec3 bMin = floor(texelIdx)*texelSize;
    vec3 bMax = ceil(texelIdx)*texelSize;

    // calc for tFar, outgoing of box
    vec3 tMin = (bMin-ro) / rd;
    vec3 tMax = (bMax-ro) / rd;
    vec3 t2 = max(tMin, tMax);
    return min(min(t2.x, t2.y), t2.z);    
}

//#define SKIP_EMPTY
vec4 conetraceAccum(vec3 ro, vec3 rd, float fov) {
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

    vec4 texel = textureLod(tVoxColor, pos, mipLevel);

    #ifdef SKIP_EMPTY
    float stepSize;
    if (texel.a == 0.0) {
        // skip color computation
        float lvl = getNonEmptyMipLevel(pos, mipLevel) - 1.0;
        stepSize = texelIntersect(pos+EPS, rd, lvl) + EPS;
    }
    else {
        // delta transmittance
        float dtm = exp( -TRANSMIT_K * texel.a );
        tm *= dtm;
        col += (1.0-dtm)*texel.rgb*tm;
        stepSize = pixSize * STEPSIZE_WRT_TEXEL;
    }
    #else
    if (texel.a > 0.0 )
    {
        float dtm = exp( -TRANSMIT_K * texel.a );
        tm *= dtm;
        col += (1.0 - dtm)*texel.rgb*tm;
    }
    float stepSize = pixSize * STEPSIZE_WRT_TEXEL;
    #endif

    // increment
    dist += stepSize;
    pos += stepSize*rd;
  }

  float alpha = 1.0-tm;
  alpha /= (1.0+AO_DIST_K*dist);
  return vec4(alpha==0 ? col : col/alpha , alpha);
}


// for AO, dist weighted transmittance accumulation
float conetraceVisibility(vec3 ro, vec3 rd, float fov) {
    vec3 pos = ro;
    float dist = 0.0;
    float pixSizeAtDist = tan(fov);

    float tm = 1.0;         // accumulated transmittance

    while(tm > TRANSMIT_MIN &&
        pos.x < 1.0 && pos.x > 0.0 &&
        pos.y < 1.0 && pos.y > 0.0 &&
        pos.z < 1.0 && pos.z > 0.0) {

        // calc mip size, clamp min to texelsize
        float pixSize = max(dist*pixSizeAtDist, gTexelSize);
        float mipLevel = max(log2(pixSize/gTexelSize), 0.0);

        // sample texture
        vec4 texel = textureLod(tVoxColor, pos, mipLevel);

        // update transmittance
        tm *= exp( -TRANSMIT_K * STEPSIZE_WRT_TEXEL*texel.a );

        // increment
        float stepSize = pixSize * STEPSIZE_WRT_TEXEL;
        dist += stepSize;
        pos += stepSize*rd;
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

    // get fragment info
    vec3 pos = vertexData.position;
    vec3 nor = normalize(vertexData.normal);
    vec4 col = getDiffuseColor(getMeshMaterial());


    //-----------------------------------------------------
    // COMPUTE COLORS
    //-----------------------------------------------------

    #define PASS_COL
    #define PASS_AO    
    #define PASS_INDIR
    #define PASS_SPEC

    vec4 cout = vec4(vec3(1.0), col.a);
    float voxelDirectionOffset = gTexelSize*ROOTTHREE;

    // if nothing there, don't color
    if ( col.a!=0.0 ) {

        #ifdef PASS_AO
        float ao = 0.0;
        {
            // setup cones constants
            // 6 cones 30 fov, 5 cones around 1 cone straight up
            #define NUM_DIRS 6.0
            #define NUM_RADIAL_DIRS 5.0
            const float FOV = radians(30.0);
            const float NORMAL_ROTATE = radians(50.0);
            const float ANGLE_ROTATE = radians(72.0);

            // radial ring of cones
            vec3 axis = findPerpendicular(nor); // find a perpendicular vector
            for (float i=0.0; i<NUM_RADIAL_DIRS; i++) {
                // rotate that vector around normal (to distribute cone around)
                vec3 rotatedAxis = rotate(axis, ANGLE_ROTATE*(i+EPS), nor);

                // ray dir is normal rotated an fov over that vector
                vec3 rd = rotate(nor, NORMAL_ROTATE, rotatedAxis);

                ao += conetraceVisibility(pos+rd*voxelDirectionOffset, rd, FOV);
            }

            // single perpendicular cone (straight up)
            ao += conetraceVisibility(pos+nor*voxelDirectionOffset, nor, FOV);

            // finally, divide
            ao /= NUM_DIRS;

            #undef NUM_DIRS
            #undef NUM_RADIAL_DIRS
        }
        #endif
        
        #ifdef PASS_INDIR
        vec4 indir = vec4(0.0);
        {
            // duplicate code from above

            #define NUM_DIRS 6.0
            #define NUM_RADIAL_DIRS 5.0
            const float FOV = radians(30.0);
            const float NORMAL_ROTATE = radians(50.0);
            const float ANGLE_ROTATE = radians(72.0);

            vec3 axis = findPerpendicular(nor);
            for (float i=0.0; i<NUM_RADIAL_DIRS; i++) {
                vec3 rotatedAxis = rotate(axis, ANGLE_ROTATE*(i+EPS), nor);
                vec3 rd = rotate(nor, NORMAL_ROTATE, rotatedAxis);
                indir += conetraceAccum(pos+rd*voxelDirectionOffset, rd, FOV);
            }

            indir += conetraceAccum(pos+nor*voxelDirectionOffset, nor, FOV);

            indir /= NUM_DIRS;

            #undef NUM_DIRS
            #undef NUM_RADIAL_DIRS
        }
        #endif
        
        #ifdef PASS_SPEC
        vec4 spec;
        {
            // single cone in reflected eye direction
            const float FOV = radians(uSpecularFOV);
            vec3 rd = normalize(pos-uCamPos);
            rd = reflect(rd, nor);
            spec = conetraceAccum(pos+rd*voxelDirectionOffset*3.0, rd, FOV);
        }
        #endif
        

        /* COMP PASSES */

        #ifdef PASS_COL
        cout = col;
        #endif
        #ifdef PASS_INDIR
            #ifdef PASS_COL
            cout.rgb += indir.rgb*indir.a;
            float difference = max(0.0,max(cout.r - 1.0, max(cout.g - 1.0, cout.b - 1.0)));
            cout.rgb = clamp(cout.rgb - difference, 0.0, 1.0);
            #else
            cout = indir;
            #endif
        #endif
        #ifdef PASS_SPEC
            #ifdef PASS_COL
            cout.rgb = mix(cout.rgb, spec.rgb*spec.a, uSpecularAmount);
            #else
            cout = spec;
            #endif
        #endif
        #ifdef PASS_AO
        cout.rgb *= ao;
        #endif
    }

    fragColor = cout;
}