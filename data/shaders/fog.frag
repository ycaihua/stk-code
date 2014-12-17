uniform sampler2D tex;

out vec4 FragColor;

vec4 getPosFromUVDepth(vec3 uvDepth, mat4 InverseProjectionMatrix);

void main()
{
    vec2 uv = 2. * gl_FragCoord.xy / screen;
    float z = texture(tex, uv).x;
    vec4 xpos = getPosFromUVDepth(vec3(uv, z), InverseProjectionMatrix);

    float dist = length(xpos.xyz);
    float factor = (1. - exp(- fog_density * dist));
    vec3 fog = fog_color * factor;

    // fog is scattering component, factor is the beer lambert absorption
    FragColor = vec4(fog, factor);
}
