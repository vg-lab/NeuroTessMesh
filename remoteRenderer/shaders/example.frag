#version 400

in vec2 uv;
out vec3 oColor;

uniform sampler2D renderedTexture;

void main( void )
{
  oColor = texture( renderedTexture, uv ).xyz;
}
