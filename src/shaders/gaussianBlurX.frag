//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

in vec2 vUV;
layout (binding = SHADOW_MAP_BINDING) uniform sampler2D shadowMap; 
layout (location = 0) out vec4 fragColor;

uniform float offset[3] = float[]( 0.0, 1.3846153846, 3.2307692308 );
uniform float weight[3] = float[]( 0.2270270270, 0.3162162162, 0.0702702703 );


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

void main(void)
{
	fragColor = texture( shadowMap, vec2(gl_FragCoord)/1024.0 ) * weight[0];
	for (int i=1; i<3; i++) {
		fragColor += texture( shadowMap, ( vec2(gl_FragCoord)+vec2(offset[i], 0.0) )/1024.0 ) * weight[i];
		fragColor += texture( shadowMap, ( vec2(gl_FragCoord)-vec2(offset[i], 0.0) )/1024.0 ) * weight[i];
	}
}
