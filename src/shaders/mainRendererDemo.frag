//---------------------------------------------------------
// MAIN RENDER DEMO
//---------------------------------------------------------

//---------------------------------------------------------
// GL IN/OUT
//---------------------------------------------------------

in block
{
    vec3 position;
    vec3 normal;
    vec3 shadowMapPos;
    vec2 uv;
    flat ivec2 propertyIndex;
} vertexData;

layout(location = 0) out vec4 fragColor;


//---------------------------------------------------------
// GLOBAL DATA
//---------------------------------------------------------

layout(binding = DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING) uniform sampler2DArray diffuseTextures[MAX_TEXTURE_ARRAYS];
layout(binding = SHADOW_MAP_BINDING) uniform sampler2D shadowMap;  

layout(binding = COLOR_TEXTURE_POSX_3D_BINDING) uniform sampler3D tVoxColorPosX;
layout(binding = COLOR_TEXTURE_NEGX_3D_BINDING) uniform sampler3D tVoxColorNegX;
layout(binding = COLOR_TEXTURE_POSY_3D_BINDING) uniform sampler3D tVoxColorPosY;
layout(binding = COLOR_TEXTURE_NEGY_3D_BINDING) uniform sampler3D tVoxColorNegY;
layout(binding = COLOR_TEXTURE_POSZ_3D_BINDING) uniform sampler3D tVoxColorPosZ;
layout(binding = COLOR_TEXTURE_NEGZ_3D_BINDING) uniform sampler3D tVoxColorNegZ;

struct MeshMaterial
{
    vec4 diffuseColor;
    vec4 specularColor;
    ivec2 diffuseTexture;
    ivec2 normalTexture;
    ivec2 specularTexture;
    float emission;
};

layout(std140, binding = MESH_MATERIAL_ARRAY_BINDING) uniform MeshMaterialArray
{
    MeshMaterial meshMaterialArray[NUM_MESHES_MAX];
};


//---------------------------------------------------------
// COMMON SHADING
//---------------------------------------------------------

MeshMaterial getMeshMaterial()
{
    int index = vertexData.propertyIndex[MATERIAL_INDEX];
    return meshMaterialArray[index];
}

vec3 getNormal(MeshMaterial material, vec3 vertexNormal)
{
    int textureId = material.normalTexture.x;
    int textureLayer = material.normalTexture.y;
    
    if(textureId == -1) // no texture
        return vertexNormal;

    // Get bump map normal
    vec3 bumpMapNormal = texture(diffuseTextures[textureId], vec3(vertexData.uv, textureLayer)).xyz;
    // Create tangent, bitangent, normal matrix
    // compute derivations of the texture coordinate
    vec2 tc_dx = dFdx(vertexData.uv);
    vec2 tc_dy = dFdy(vertexData.uv);
    vec3 p_dx  = dFdx(vertexData.position);
    vec3 p_dy  = dFdy(vertexData.position);
    // compute initial tangent and bi-tangent
    vec3 t = normalize( tc_dy.y * p_dx - tc_dx.y * p_dy );
    vec3 b = normalize( tc_dy.x * p_dx - tc_dx.x * p_dy ); // sign inversion
    // get new tangent from a given mesh normal
    vec3 n = vertexNormal;
    vec3 x = cross(n, t);
    t = cross(x, n);
    t = normalize(t);
    // get updated bi-tangent
    x = cross(b, n);
    b = cross(n, x);
    b = normalize(b);
    mat3 tbn = mat3(t, b, n);

    // Converted bump map normal to [-1,1] range
    bumpMapNormal = 2.0 * bumpMapNormal - vec3(1.0, 1.0, 1.0);
    vec3 newNormal = tbn * bumpMapNormal;
    newNormal = normalize(newNormal);
    return newNormal;
}

vec4 getDiffuseColor(MeshMaterial material)
{
    int textureId = material.diffuseTexture.x;
    int textureLayer = material.diffuseTexture.y;

    if(textureId == -1) // no texture
        return material.diffuseColor;
    
    vec4 diffuseColor = texture(diffuseTextures[textureId], vec3(vertexData.uv, textureLayer));
    
    if(diffuseColor.a == 0.0) // no alpha = invisible and discarded
        discard;
    return diffuseColor;
}

vec3 getSpecularColor(MeshMaterial material)
{
    int textureId = material.specularTexture.x;
    int textureLayer = material.specularTexture.y;

    if(textureId == -1) // no texture
        return material.specularColor.rgb;
    else
        return texture(diffuseTextures[textureId], vec3(vertexData.uv, textureLayer)).rgb;
}

float getVisibility()
{
	float fragLightDepth = vertexData.shadowMapPos.z;
    float shadowMapDepth = texture(shadowMap, vertexData.shadowMapPos.xy).r;

    if(fragLightDepth <= shadowMapDepth)
        return 1.0;

    // Less darknessFactor means lighter shadows
    float darknessFactor = 20.0;
    return clamp(exp(darknessFactor * (shadowMapDepth - fragLightDepth)), 0.0, 1.0);
}


//---------------------------------------------------------
// SHADER VARS
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

#define STEPSIZE_WRT_TEXEL 0.3333  // Cyril uses 1/3
#define STEPSIZE_EMPTY_WRT_TEXEL 0.75 // when alpha = 0.0
#define TRANSMIT_MIN 0.05
#define TRANSMIT_K  8.0
#define INDIR_DIST_K 4.0
#define INDIR_K 1.5
#define AO_DIST_K 0.3
#define JITTER_K 0.1

vec3 gNormal, gDiffuse, gSpecular;
float gTexelSize, gRandVal, gNumMipMaps;


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

// assumption, current pos is empty (alpha = 0.0)
float getNonEmptyMipLevel(vec3 pos, float mipLevel) {
    float level = ceil(mipLevel);
    float alpha = 0.0;

    do {
        ++level;
        alpha = textureLod(tVoxColorPosX, pos, level).a;
    } while(level < gNumMipMaps && alpha <= 0.0);

    return level;
}

// params: origin and ray within texture space
// intersect outside bound of the enclosing texel of given mip level
float texelIntersect(vec3 ro, vec3 rd, float mipLevel) {
    float texelSize = pow(2, mipLevel) / uVoxelRes;

    vec3 texelIdx = ro/texelSize;

    vec3 bMin = floor(texelIdx)*texelSize;
    vec3 bMax = ceil(texelIdx)*texelSize;

    // calc for tFar, outgoing of box
    vec3 tMin = (bMin-ro) / rd;
    vec3 tMax = (bMax-ro) / rd;
    vec3 t2 = max(tMin, tMax);
    return min(min(t2.x, t2.y), t2.z);    
}

vec4 sampleAnisotropic(vec3 pos, vec3 dir, float mipLevel) {
    vec4 xtexel = dir.x > 0.0 ? 
        textureLod(tVoxColorNegX, pos, mipLevel) : 
        textureLod(tVoxColorPosX, pos, mipLevel);
    
    vec4 ytexel = dir.y > 0.0 ? 
        textureLod(tVoxColorNegY, pos, mipLevel) : 
        textureLod(tVoxColorPosY, pos, mipLevel);
    
    vec4 ztexel = dir.z > 0.0 ? 
        textureLod(tVoxColorNegZ, pos, mipLevel) : 
        textureLod(tVoxColorPosZ, pos, mipLevel);
    
    // get scaling factors for each axis
    dir = abs(dir);
    //dir = dir/( dir.x + dir.y + dir.z);
    
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

        //float vocc = textureLod(tVoxColorPosX, pos, mipLevel).a;
        vec4 vocc = sampleAnisotropic(pos, rd, mipLevel);
        
        float stepSize;
        if (vocc.a <= 0.0) {
            float lvl = getNonEmptyMipLevel(pos, mipLevel) - 2.0;
            stepSize = texelIntersect(pos, rd, lvl) + EPS;
            //stepSize = pixSize * STEPSIZE_EMPTY_WRT_TEXEL;
        }
        else {
            float dtm = exp( -TRANSMIT_K * STEPSIZE_WRT_TEXEL * vocc.a );
            tm *= dtm;
            col += (1.0-dtm) * vocc.rgb * tm * 10.0;

            stepSize = pixSize * STEPSIZE_WRT_TEXEL;
        }
          
        // increment        
        stepSize += gRandVal*pixSize*JITTER_K;
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

        //vec4 vocc = textureLod(tVoxColorPosX, pos, mipLevel);
        vec4 vocc = sampleAnisotropic(pos, rd, mipLevel);

        float stepSize;
        if (vocc.a <= 0.0) {
            //float lvl = getNonEmptyMipLevel(pos, mipLevel) - 1.0;
            //stepSize = texelIntersect(pos, rd, lvl) + EPS;
            stepSize = pixSize * STEPSIZE_EMPTY_WRT_TEXEL;
        }
        else {
            float dtm = exp( -TRANSMIT_K * STEPSIZE_WRT_TEXEL * vocc.a );
            tm *= dtm;

            // calc local illumination
            vec3 lightCol = vocc.rgb * (1.0-dtm) * tm;
            //lightCol *= (INDIR_DIST_K*dist)*(INDIR_DIST_K*dist);
            col.rgb += lightCol;

            stepSize = pixSize * STEPSIZE_WRT_TEXEL;
        }

        // increment
        stepSize += gRandVal*pixSize*JITTER_K;
        dist += stepSize;
        pos += stepSize*rd;
    }
  
    // weight AO by distance f(r) = 1/(1+K*r)
    float visibility = min( tm*(1.0+AO_DIST_K*dist*dist)*5.0, 1.0);

    return vec4(col.rgb, visibility);
    //return vec4(visibility);
}

void main()
{
    // current vertex info
    vec3 worldPos = vertexData.position;
    MeshMaterial material = getMeshMaterial();
    gNormal = getNormal(material, normalize(vertexData.normal));
    gDiffuse = getDiffuseColor(material).rgb;
    gSpecular = getSpecularColor(material);

    vec3 pos = (worldPos-uVoxelRegionWorld.xyz)/uVoxelRegionWorld.w;    // in tex coords

    float fadeX = min(max(pos.x - 0.0,0.0),max(1.0 - pos.x,0.0));
    float fadeY = min(max(pos.y - 0.0,0.0),max(1.0 - pos.y,0.0));
    float fadeZ = min(max(pos.z - 0.0,0.0),max(1.0 - pos.z,0.0));
    float fade = min(fadeX, min(fadeY, fadeZ));
    fade = min(fade * 5.0, 1.0);

    vec3 cout = vec3(0.0);

    // calc globals
    gRandVal = 0;//rand(pos.xy+pos.z);
    gNumMipMaps = log2(uVoxelRes)+1.0;
    gTexelSize = 1.0/uVoxelRes; // size of one texel in normalized texture coords
    float voxelOffset = gTexelSize*2.5;

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
        vec3 rd = normalize(worldPos-uCamPos);
        rd = reflect(rd, gNormal);
        spec = conetraceSpec(pos+rd*voxelOffset, rd, FOV);
    }
    #endif

    #ifdef PASS_DIFFUSE

    float visibility = min(getVisibility(), 1.0);
    vec3 view = normalize(worldPos-uCamPos);
    float diffuseTerm = max(dot(uLightDir, gNormal), 0.0);
    cout += gDiffuse.rgb * 0.4 * uLightColor * diffuseTerm * visibility * min(1.0-fade,1.0);
    cout += gDiffuse.rgb * 0.2 * min(1.0-fade,1.0);
        
    #define SPEC 0.1
    vec3 reflectedLight = reflect(uLightDir, gNormal);
    vec3 halfAngle = normalize(uLightDir - view);
    float angleNormalHalf = acos(dot(halfAngle, gNormal));
    float exponent = angleNormalHalf / SPEC;
    exponent = -(exponent * exponent);
    float specularTerm = exp(exponent);
    cout += uLightColor * gSpecular * specularTerm * visibility * min(1.0-fade,1.0) * 0.2;

    #endif
    #ifdef PASS_INDIR
    cout += indir.rgb*10.0*gDiffuse*fade;
    //cout *= indir.a;
    #endif
    #ifdef PASS_SPEC
    cout = mix(cout, spec*fade, uSpecularAmount);
    #endif

    // adjust blown out colors
    //float difference = max(0.0,max(cout.r - 1.0, max(cout.g - 1.0, cout.b - 1.0)));
    //cout = clamp(cout - difference, 0.0, 1.0);

    // If emissive, ignore shading and just draw diffuse color
    cout = mix(cout, gDiffuse.rgb, material.emission);

    fragColor = vec4(cout, 1.0);
}