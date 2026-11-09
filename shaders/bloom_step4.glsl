#version 330

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec2 direction;
uniform float sigma;

out vec4 finalColor;

void main(){
    int radius = min(int(ceil(sigma * 3.0)), 64);
    float k = -0.5 / (sigma * sigma);

    vec3 max_v = vec3(0.0);
    for(int i = -radius; i <= radius; i++){
        float w = exp(k * float(i * i));
        max_v = max(max_v, w * texture(texture0, fragTexCoord + direction * float(i)).rgb);
    }
    finalColor = vec4(max_v, 1.0);
}
