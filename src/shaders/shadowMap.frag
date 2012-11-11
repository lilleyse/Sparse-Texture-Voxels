//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

in block
{
    float depth;
} vertOutput;

layout (location = 0) out vec4 fragColor;


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

void main()
{    
    float depth = vertOutput.depth + .01;

    // Apply depth bias to avoid some z-fighting (maybe optional);
    float dx = dFdx(depth);
	float dy = dFdy(depth);
    depth += abs(dx) + abs(dy);

    fragColor = vec4(depth, 0.0, 0.0, 0.0);
}
