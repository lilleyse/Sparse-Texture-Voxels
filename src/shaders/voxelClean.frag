//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout(location = 0) out vec4 fragColor;
layout(binding = VOXEL_DIRECTIONS_IMAGE_ARRAY_BINDING, rgba8) writeonly uniform image3D tDirectionalVoxels[NUM_VOXEL_DIRECTIONS];

flat in int slice;

//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

void main()
{
    ivec3 globalId = ivec3(ivec2(gl_FragCoord.xy), slice);
    vec4 finalColor = vec4(0.0);
    imageStore(tDirectionalVoxels[0], globalId, finalColor);
    imageStore(tDirectionalVoxels[1], globalId, finalColor);
    imageStore(tDirectionalVoxels[2], globalId, finalColor);
    imageStore(tDirectionalVoxels[3], globalId, finalColor);
    imageStore(tDirectionalVoxels[4], globalId, finalColor);
    imageStore(tDirectionalVoxels[5], globalId, finalColor);
}