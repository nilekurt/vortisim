#version 150

in vec2 in_position;

uniform mat4 mvp;

out mat4 frag_mvp;

void
main()
{
    frag_mvp = mvp;
    gl_Position = mvp * vec4(in_position, 0.0, 1.0);
}
