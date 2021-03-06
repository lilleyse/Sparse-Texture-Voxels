//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout (location = 0, index = 0) out vec4 fragColor;

in block
{
    vec3 color;
} vertexData;


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

void main()
{
    fragColor = vec4(vertexData.color, 1.0);
}