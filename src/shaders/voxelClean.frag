//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout(location = 0) out vec4 fragColor;

layout(binding = COLOR_IMAGE_POSX_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorPosX;
layout(binding = COLOR_IMAGE_NEGX_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorNegX;
layout(binding = COLOR_IMAGE_POSY_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorPosY;
layout(binding = COLOR_IMAGE_NEGY_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorNegY;
layout(binding = COLOR_IMAGE_POSZ_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorPosZ;
layout(binding = COLOR_IMAGE_NEGZ_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorNegZ;

flat in int slice;

//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

void main()
{
    ivec3 globalId = ivec3(ivec2(gl_FragCoord.xy), slice);
    vec4 finalColor = vec4(0.0);
    imageStore(tVoxColorPosX, globalId, finalColor);
    imageStore(tVoxColorNegX, globalId, finalColor);
    imageStore(tVoxColorPosY, globalId, finalColor);
    imageStore(tVoxColorNegY, globalId, finalColor);
    imageStore(tVoxColorPosZ, globalId, finalColor);
    imageStore(tVoxColorNegZ, globalId, finalColor);
}