#version 330 core
in vec3 vDir;
out vec4 FragColor;

void main()
{
    // hard-coded blue sky gradient
    float t = normalize(vDir).y * 0.5 + 0.5;
    vec3 sky = mix(vec3(0.6, 0.8, 1.0), vec3(0.2, 0.4, 0.8), t);
    FragColor = vec4(sky, 1.0);
}
