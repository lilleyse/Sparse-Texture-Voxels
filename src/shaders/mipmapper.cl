constant sampler_t sampler =    CLK_NORMALIZED_COORDS_TRUE  |
                                CLK_ADDRESS_NONE            |
                                CLK_FILTER_NEAREST;


inline int indexConverter(int sideLength, int4 index3d)
{
    return index3d.x + index3d.y*sideLength + index3d.z*sideLength*sideLength;
}

__kernel void colorMipmapper(read_only image3d_t textureBase, int sideLength, __global uchar4* textureMipsPBO)
{
    //float4 summedColor = (float4)(0.0,1.0,0.0,1.0);

    int4 index = (int4)(get_global_id(0), get_global_id(1), get_global_id(2), sideLength-1);
    int indexMip = indexConverter(sideLength, index);
    int4 index3d = index*2;
    /*for(int i = 0; i < 2; i++)
    for(int j = 0; j < 2; j++)
    for(int k = 0; k < 2; k++)
    {
        int4 neighbor = index3d + (int4)(i,j,k,0);
        float4 color = read_imagef(textureBase, sampler, neighbor);
        color.xyz *= color.w;
        summedColor += color;
    }

    summedColor.xyz /= summedColor.w;
    summedColor.w /= 8.0;*/
    //float4 summedColor = read_imagef(textureBase, sampler, (int4)(62,62,62,0));//index3d);
    //uchar4 finalColor = convert_uchar4(summedColor*255);
    //textureMipsPBO[indexMip] = finalColor;
    textureMipsPBO[indexMip] = convert_uchar4(index3d*4);
};
