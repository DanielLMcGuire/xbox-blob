R"GLSL(#version 300 es

layout(location = 0) in vec3 vertexPosition;

uniform mat4 mvp;
uniform mat4 model;
uniform vec3 cameraPos;
uniform float fogRadius;
uniform float cameraRadiusFromBlob;
uniform int isBackdrop;

out float v_depthVal;

void main()
{
    if (isBackdrop == 1)
    {
        v_depthVal = 1.0;
        gl_Position = vec4(vertexPosition.xy, 1.0, 1.0);
    }
    else
    {
        vec3 worldPos = (model * vec4(vertexPosition, 1.0)).xyz;
        float viewDist = length(worldPos - cameraPos);

        v_depthVal = clamp((viewDist - cameraRadiusFromBlob + fogRadius) / (2.0 * fogRadius), 0.0, 1.0);
        gl_Position = mvp * vec4(vertexPosition, 1.0);
    }
}
)GLSL"