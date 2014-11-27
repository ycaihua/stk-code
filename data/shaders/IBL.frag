uniform sampler2D ntex;
uniform sampler2D dtex;
uniform sampler2D ctex;
uniform sampler2D emittex;

out vec4 FragColor;

vec3 DecodeNormal(vec2 n);
vec4 getPosFromUVDepth(vec3 uvDepth, mat4 InverseProjectionMatrix);
vec3 DiffuseIBL(vec3 normal, vec3 color);
vec3 SpecularIBL(vec3 normal, vec3 V, float roughness);

void main(void)
{
    vec2 uv = gl_FragCoord.xy / screen;
    vec3 normal = normalize(DecodeNormal(2. * texture(ntex, uv).xy - 1.));
    vec3 color = texture(ctex, uv).rgb;

    vec3 emitcolor = pow(color.rgb, vec3(3.)) * 15 * texture(emittex, uv).r;

    vec3 diffuse = DiffuseIBL(normal, color);

    float z = texture(dtex, uv).x;

    vec4 xpos = getPosFromUVDepth(vec3(uv, z), InverseProjectionMatrix);
    vec3 eyedir = -normalize(xpos.xyz);
    float specval = texture(ntex, uv).z;
    vec3 specular = color * SpecularIBL(normal, eyedir, specval);
    float reflectance = texture(ntex, uv).a;

    FragColor = vec4(.2 * ((1. - reflectance) * diffuse + reflectance * specular) + emitcolor, texture(ctex, uv).a);
}
