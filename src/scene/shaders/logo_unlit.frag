#version 330

in vec2 fragTexCoord;

uniform sampler2D tex0;

out vec4 finalColor;

void main()
{
    if (fragTexCoord.x < 0.0 || fragTexCoord.x > 1.0 || fragTexCoord.y < 0.0 || fragTexCoord.y > 1.0)
    {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    finalColor = texture(tex0, fragTexCoord);
}
