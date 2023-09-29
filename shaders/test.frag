#version 450
#define PI 3.14159265359

// Push constants
layout (push_constant, std430) uniform pc{
    float time;
};

layout (location = 0) in vec3 outNorm;
layout (location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(outNorm, 1.0) * (0.5+0.5*sin(time*(PI/180.0)/5.0));
}