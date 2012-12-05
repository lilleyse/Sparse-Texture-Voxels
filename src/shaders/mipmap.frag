//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout(location = 0) out vec4 fragColor;
layout(binding = VOXEL_DIRECTIONS_ARRAY_BINDING) uniform sampler3D tDirectionalVoxelTextures[MAX_VOXEL_TEXTURES];
layout(binding = VOXEL_DIRECTIONS_IMAGE_ARRAY_BINDING, rgba8) writeonly uniform image3D tDirectionalVoxels[NUM_VOXEL_DIRECTIONS];

flat in int slice;


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

// alpha blend RGB, average Alpha
vec4 alphaBlend(vec4 front, vec4 back)
{
    front.rgb += (1.0-front.a)*back.rgb;
    //front.a += (1.0-front.a)*back.a;
    front.a = (front.a+back.a)/2.0; // alpha not blended, just averaged
    return front;
}

vec4 calcDirectionalColor(
    vec4 front1, vec4 front2, vec4 front3, vec4 front4, 
    vec4 back1, vec4 back2, vec4 back3, vec4 back4)
{
    vec4 color1 = alphaBlend(front1, back1);
    vec4 color2 = alphaBlend(front2, back2);
    vec4 color3 = alphaBlend(front3, back3);
    vec4 color4 = alphaBlend(front4, back4);
    color1.rgb *= color1.a;
    color2.rgb *= color2.a;
    color3.rgb *= color3.a;
    color4.rgb *= color4.a;
    vec4 color = color1 + color2 + color3 + color4;
    color.rgb /= color.a;
    color.a /= 4.0;
    return color;
}

int mipLevel;
ivec3 v000, v100, v010, v001, v110, v011, v101, v111;
vec4 getDirectionalColor (int offset)
{
    return calcDirectionalColor(
        texelFetch(tDirectionalVoxelTextures[offset], v100, mipLevel),
        texelFetch(tDirectionalVoxelTextures[offset], v101, mipLevel),
        texelFetch(tDirectionalVoxelTextures[offset], v110, mipLevel),
        texelFetch(tDirectionalVoxelTextures[offset], v111, mipLevel),
        texelFetch(tDirectionalVoxelTextures[offset], v000, mipLevel),
        texelFetch(tDirectionalVoxelTextures[offset], v001, mipLevel),
        texelFetch(tDirectionalVoxelTextures[offset], v010, mipLevel),
        texelFetch(tDirectionalVoxelTextures[offset], v011, mipLevel)
    );
}

void main()
{
    mipLevel = uCurrentMipLevel-1;
    ivec3 globalId = ivec3(ivec2(gl_FragCoord.xy), slice);
    ivec3 oldGlobalId = globalId*2;

    v000 = oldGlobalId + ivec3(0,0,0);
    v100 = oldGlobalId + ivec3(1,0,0);
    v010 = oldGlobalId + ivec3(0,1,0);
    v001 = oldGlobalId + ivec3(0,0,1);
    v110 = oldGlobalId + ivec3(1,1,0);
    v011 = oldGlobalId + ivec3(0,1,1);
    v101 = oldGlobalId + ivec3(1,0,1);
    v111 = oldGlobalId + ivec3(1,1,1);
        
    // fill the color values
    imageStore(tDirectionalVoxels[0], globalId, getDirectionalColor(0));
    imageStore(tDirectionalVoxels[1], globalId, getDirectionalColor(1));
    imageStore(tDirectionalVoxels[2], globalId, getDirectionalColor(2));
    imageStore(tDirectionalVoxels[3], globalId, getDirectionalColor(3));
    imageStore(tDirectionalVoxels[4], globalId, getDirectionalColor(4));
    imageStore(tDirectionalVoxels[5], globalId, getDirectionalColor(5));
}