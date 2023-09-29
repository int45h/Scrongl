#!/bin/bash

glslangValidator -V -S vert shaders/test.vert -o shaders/test_vsh.h --vn test_vsh_spirv
glslangValidator -V -S frag shaders/test.frag -o shaders/test_fsh.h --vn test_fsh_spirv
