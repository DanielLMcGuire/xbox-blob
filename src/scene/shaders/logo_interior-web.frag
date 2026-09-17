#version 300 es
precision mediump float;

in vec2 fragTexCoord;

uniform vec4 gradStart;
uniform vec4 gradEnd;
uniform vec4 flatStart;
uniform vec4 flatEnd;
uniform float blendW;

out vec4 finalColor;

void main()
{
    vec3 gradColor = mix(gradStart.rgb, gradEnd.rgb, fragTexCoord.y);
    vec3 flatColor = mix(flatStart.rgb, flatEnd.rgb, fragTexCoord.y);
    finalColor = vec4(mix(gradColor, flatColor, blendW), 1.0);
}