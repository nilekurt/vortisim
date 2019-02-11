#version 150

in vec2 in_position;

uniform mat4 mvp;

void main()
{
    gl_PointSize = 5.0;
    gl_Position = mvp * vec4(in_position, 0.0, 1.0);
}
