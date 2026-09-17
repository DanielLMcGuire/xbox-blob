R"GLSL(#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord0;
layout(location = 2) in vec2 vertexTexCoord1;

uniform vec2 plasmaOffset[3];
uniform vec2 plasmaScale[3];

out vec2 fragTexCoord0;
out vec2 fragPlasmaUV[3];

void main()
{
    fragTexCoord0 = vertexTexCoord0;

    for (int i = 0; i < 3; i++)
        fragPlasmaUV[i] = vertexTexCoord1 * plasmaScale[i] + plasmaOffset[i];

    gl_Position = vec4(vertexPosition.xy, 0.0, 1.0);
}
)GLSL"