#version 330

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec2 texel;

out vec4 finalColor;

void main(){
    vec3 acc  = texture(texture0, fragTexCoord + vec2(-1.0, -1.0) * texel).rgb
              + texture(texture0, fragTexCoord + vec2( 1.0, -1.0) * texel).rgb
              + texture(texture0, fragTexCoord + vec2(-1.0,  1.0) * texel).rgb
              + texture(texture0, fragTexCoord + vec2( 1.0,  1.0) * texel).rgb;
    finalColor = vec4(acc * 0.25, 1.0);
}
