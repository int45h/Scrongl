#version 450

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNorm;
layout (location = 2) in vec3 vUV;

layout (location = 0) out vec3 outNorm;

layout (set = 0, binding = 0) uniform UBO{
    mat4 model;
};

void main()
{
    gl_Position = model * vec4(vPos, 1.0);
    outNorm = vNorm;
}