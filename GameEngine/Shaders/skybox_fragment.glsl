#version 330 core
in vec3 vDir;
out vec4 FragColor;

uniform vec3 bottomColor;
uniform vec3 topColor;

void main()
{
    float t = normalize(vDir).y * 0.5 + 0.5;
    vec3 sky = mix(bottomColor, topColor, t);
    FragColor = vec4(sky, 1.0);
}
