#version 330

in vec2 fragTexCoord0;
in vec2 fragPlasmaUV[3];

uniform sampler2D intensityMap;
uniform sampler2D plasmaMap0;
uniform sampler2D plasmaMap1;
uniform sampler2D plasmaMap2;
uniform bool useIntensityMap;
uniform vec3 intensityTint;
uniform vec3 glowColor;

out vec4 finalColor;

void main()
{
    float p0 = texture(plasmaMap0, fragPlasmaUV[0]).a;
    float p1 = texture(plasmaMap1, fragPlasmaUV[1]).a;
    float p2 = texture(plasmaMap2, fragPlasmaUV[2]).a;
    vec3 plasmaSum = vec3(p0 + p1 + p2) * intensityTint;

    vec3 occlusion = useIntensityMap ? texture(intensityMap, fragTexCoord0).rgb : vec3(1.0);

    vec3 color = plasmaSum * occlusion + glowColor;
    finalColor = vec4(color, 1.0);
}
