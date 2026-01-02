#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 VP;
out vec3 vDir;

void main()
{
    vDir = aPos;
    vec4 pos = VP * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // force depth = 1.0
}
