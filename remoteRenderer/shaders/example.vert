#version 400

in vec2 inPosition;
out vec2 uv;

void main( void )
{
  uv = (inPosition + vec2( 1.0, 1.0 )) * 0.5;
gl_Position = vec4( inPosition,  0.1, 1.0 );
}
