#version 330

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform sampler2D history;
uniform sampler2D glow_history;
uniform float decay;
uniform float residual;

out vec4 finalColor;

const vec3 LUMA = vec3(0.2126, 0.7152, 0.0722);

void main(){
    vec3 curr = texture(texture0, fragTexCoord).rgb;
    vec3 anterior = texture(history, fragTexCoord).rgb;

    vec3 diff = (curr - anterior) * 255.0;
    float intensity = min(dot(diff, diff), 32.0 * 32.0) / (32.0 * 32.0);
    float luma = max(dot(curr, LUMA), 0.02);

    vec3 brightness = curr * pow(intensity, 4.0) * 2.0 / luma;
    vec3 glow_anterior = texture(glow_history, fragTexCoord).rgb * decay;
    vec3 residual_glow = curr * residual;
    finalColor = vec4(max(max(brightness, residual_glow), glow_anterior), 1.0);
}
