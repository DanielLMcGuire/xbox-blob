R"GLSL(#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 3) in vec3 vertexNormal;

uniform mat4 mvp;
uniform mat4 model;
uniform mat3 normalMatrix;
uniform mat4 lightSpaceMatrix;

out vec3 fragWorldPos;
out vec3 fragNormal;
out vec4 fragShadowCoord;

void main()
{
    vec4 worldPos = model * vec4(vertexPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    fragNormal = normalMatrix * vertexNormal;
    fragShadowCoord = lightSpaceMatrix * worldPos;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)GLSL"