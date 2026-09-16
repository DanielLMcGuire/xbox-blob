#version 330

in float viewDist;

uniform float fogRadius;
uniform float cameraRadiusFromBlob;
uniform vec2 targetSize;

out vec4 finalColor;

void main()
{
    float depthVal = clamp((viewDist - cameraRadiusFromBlob + fogRadius) / (2.0 * fogRadius), 0.0, 1.0);

    vec2 ndc = (gl_FragCoord.xy / targetSize) * 2.0 - 1.0;
    float r2 = dot(ndc, ndc);
    float vign = clamp((1.0 - r2) * (1.0 - r2), 0.0, 1.0);

    float v = depthVal * vign;
    finalColor = vec4(v, v, v, 1.0);
}
