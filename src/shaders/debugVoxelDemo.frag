//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout (location = 0, index = 0) out vec4 fragColor;

in block
{
    vec3 position;
    vec4 color;
    vec3 normal;

} vertexData;


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

void main()
{
    fragColor = vertexData.color;
}