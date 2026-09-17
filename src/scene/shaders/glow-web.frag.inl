R"GLSL(#version 300 es
precision mediump float;

in vec2 fragTexCoord;

uniform sampler2D tex0;
uniform vec4 tint;

out vec4 finalColor;

void main()
{
    vec4 texColor = texture(tex0, fragTexCoord);
    finalColor = vec4(texColor.rgb * tint.rgb * tint.a, texColor.a);
}
)GLSL"