//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout(location = POSITION_ATTR) in vec3 position;
layout(location = DEBUG_TRANSFORM_ATTR) in vec4 transformation;
layout(location = DEBUG_COLOR_ATTR) in vec3 color[6];

out block
{
    vec3 color;

} vertexData;

out gl_PerVertex
{
    vec4 gl_Position;
};


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

void main()
{
    // Create the model matrix
    float scale = transformation.w;
    mat4 modelMatrix = mat4(scale);
    modelMatrix[3] = vec4(transformation.xyz, 1.0);

    vec4 worldPosition = modelMatrix * vec4(position, 1.0);

    // Caluclate the clip space position
    gl_Position = uViewProjection * worldPosition;
    
    vertexData.color = color[gl_VertexID/4];
}