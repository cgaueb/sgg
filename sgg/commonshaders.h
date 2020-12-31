#pragma once

const char* __PrimitivesVertexShader = R"(
#version 120

attribute vec4 coord;
varying vec2 texcoord;
uniform mat4 MV;
uniform mat4 P;

void main(void) {
  gl_Position = P*MV*vec4(coord.xy, 0, 1);
  texcoord = coord.zw;
}
)";


const char* __SolidFragmentShader = R"(
#version 120

varying vec2 texcoord;
uniform vec4 color1;
uniform vec4 color2;
uniform sampler2D tex;
uniform int has_texture;
uniform vec2 gradient;

void main(void) {
	vec4 color = mix( color1, color2, dot(texcoord,gradient));
	vec4 tex_color = texture2D(tex, texcoord); 
	if (has_texture>0)
		gl_FragColor = color * tex_color;
	else
		gl_FragColor = color;
}
)";
