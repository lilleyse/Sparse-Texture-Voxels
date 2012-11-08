//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout(location = POSITION_ATTR) in vec3 position;
layout(location = NORMAL_ATTR) in vec3 normal;
layout(location = DEBUG_TRANSFORM_ATTR) in vec4 transformation;
layout(location = DEBUG_COLOR_ATTR) in vec4 color;

out block
{
    vec3 position;
    vec4 color;
    vec3 normal;

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
    
    vertexData.position = vec3(worldPosition);
    vertexData.color = color;
    vertexData.normal = normal;
}