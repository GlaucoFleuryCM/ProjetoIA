#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform sampler2D thin_bloom;
uniform sampler2D wide_bloom;

uniform float time;    
uniform vec2 resolution; 
uniform float progress;

out vec4 finalColor;

const vec3 LUMA = vec3(0.2126, 0.7152, 0.0722);

vec3 dessat_clamp(vec3 c){
    float luma = dot(c, LUMA);
    if(luma >= 1.0) return vec3(1.0);
    if(luma <= 0.0) return vec3(0.0);

    float sat = 1.0;
    for(int i = 0; i < 3; i++){
        if(c[i] > 1.0) sat = min(sat, (luma - 1.0) / (luma - c[i]));
        else if(c[i] < 0.0) sat = min(sat, luma / (luma - c[i]));
    }
    return clamp((c - luma) * sat + luma, 0.0, 1.0);
}

void main(){
    vec4 color = texture(texture0, fragTexCoord);

    vec2 pixel = 1.0 / resolution;
    vec3 brightness = vec3(0.0);
    for(int x = -3; x <= 3; x++){
        for(int y = -3; y <= 3; y++){
            brightness += texture(texture0, fragTexCoord + vec2(x, y) * pixel * 1.5).rgb;
        }
    }
    brightness /= 49.0;
    color.rgb += brightness * 0.6;

    color.rgb += texture(thin_bloom, fragTexCoord).rgb * 0.15
             + texture(wide_bloom, fragTexCoord).rgb * 0.05;
    color.rgb = dessat_clamp(color.rgb);

    finalColor = color * colDiffuse * fragColor;
}
