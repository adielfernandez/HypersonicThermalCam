#version 120

uniform sampler2DRect tex0;
uniform float contrastExp;
uniform float contrastPhase;

void main(){
	//this is the fragment shader
	//this is where the pixel level drawing happens
	//gl_FragCoord gives us the x and y of the current pixel its drawing
//    vec2 pos = gl_TexCoord[0].st;
//    vec4 col = texture2DRect(tex0, pos);
    
    gl_FragColor = vec4(contrastPhase, contrastPhase, contrastPhase, contrastPhase);


	
}