#ifdef Use_Bindless_Texture
layout(bindless_sampler) uniform sampler2D tex;
#else
uniform sampler2D tex;
#endif

in vec2 uv;
in vec3 nor;
in vec4 color;
out vec4 FragColor;

vec3 DiffuseIBL(vec3 normal, vec3 V, float roughness, vec3 color);
vec3 SpecularIBL(vec3 normal, vec3 V, float roughness, vec3 F0);
vec3 SpecularBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness);
vec3 DiffuseBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness);
vec3 SunMRP(vec3 normal, vec3 eyedir);

void main()
{
    vec4 diffusecolor = texture(tex, uv);
#ifdef Use_Bindless_Texture
    diffusecolor.xyz = pow(diffusecolor.xyz, vec3(2.2));
#endif
    diffusecolor.xyz *= pow(color.xyz, vec3(2.2));
    diffusecolor.a *= color.a;

    vec3 tmp = vec3(gl_FragCoord.xy / screen, gl_FragCoord.z);
    tmp = 2. * tmp - 1.;
    vec4 xpos = vec4(tmp, 1.0);
    xpos = InverseProjectionMatrix * xpos;
    xpos.xyz /= xpos.w;

    vec3 normal = normalize(nor);
    vec3 eyedir = -normalize(xpos.xyz);

    vec3 Lightdir = SunMRP(normal, eyedir);
    float NdotL = clamp(dot(normal, Lightdir), 0., 1.);

    diffusecolor.rgb =
        0.2 * (DiffuseIBL(normal, eyedir, 0., diffusecolor.rgb) + SpecularIBL(normal, eyedir, 0., vec3(0.04))) +
        sun_col * NdotL * (DiffuseBRDF(normal, eyedir, Lightdir, diffusecolor.rgb, 0.) + SpecularBRDF(normal, eyedir, Lightdir, vec3(.04), 0.));

    float dist = length(xpos.xyz);
    float factor = (1. - exp(- fog_density * dist));
    vec3 fog = fog_color * factor;

    FragColor = vec4((fog + diffusecolor.rgb) * diffusecolor.a, diffusecolor.a);
}
