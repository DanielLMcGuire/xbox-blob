#version 300 es
precision mediump float;

in vec3 fragNormal;
in vec3 fragEyeDir;

uniform vec4 baseColor;
uniform vec4 ambientColor;
uniform float alphaScale;

out vec4 finalColor;

void main()
{
    vec3 n = normalize(fragNormal);
    vec3 e = normalize(fragEyeDir);

    float ndote = clamp(dot(n, e), 0.0, 1.0);
    float rim = (1.0 - ndote);
    rim = rim * rim;
    float keep = 1.0 - rim;

    vec4 c = keep * baseColor;
    c.rgb += ambientColor.rgb;
    c.a *= alphaScale;

    finalColor = c;
}