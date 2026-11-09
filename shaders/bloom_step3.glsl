#version 330

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform sampler2D history;
uniform float weight;

out vec4 finalColor;

void main(){
    vec3 curr = texture(texture0, fragTexCoord).rgb;
    vec3 prev = texture(history, fragTexCoord).rgb;
    finalColor = vec4(mix(prev, curr, weight), 1.0);
}
