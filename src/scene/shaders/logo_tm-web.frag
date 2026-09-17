#version 300 es
precision mediump float;

in vec2 fragTexCoord;

uniform sampler2D tex0;
uniform float fadeAlpha;

out vec4 finalColor;

void main()
{
    vec4 texColor = texture(tex0, fragTexCoord);
    finalColor = vec4(texColor.rgb, texColor.a * fadeAlpha);
}