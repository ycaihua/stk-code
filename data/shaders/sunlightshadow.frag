uniform sampler2D ntex;
uniform sampler2D dtex;
uniform sampler2D ctex;
uniform sampler2DArrayShadow shadowtex;

uniform float split0;
uniform float split1;
uniform float split2;
uniform float splitmax;
uniform float shadow_res;

in vec2 uv;
out vec4 FragColor;

vec3 DecodeNormal(vec2 n);
vec3 SpecularBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness);
vec3 DiffuseBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness);
vec4 getPosFromUVDepth(vec3 uvDepth, mat4 InverseProjectionMatrix);
vec3 SunMRP(vec3 normal, vec3 eyedir);

float getShadowFactor(vec3 pos, int index)
{
    vec4 shadowcoord = (InverseViewMatrix * vec4(pos, 1.0));
    shadowcoord /= shadowcoord.w;
    shadowcoord = ShadowViewProjMatrixes[index] * shadowcoord;
    shadowcoord /= shadowcoord.w;
    vec2 shadowtexcoord = shadowcoord.xy * 0.5 + 0.5;
    float d = .5 * shadowcoord.z + .5;

    float result = 0.;

    for (float i = -1.; i <= 1.; i += 1.)
    {
        for (float j = -1.; j <= 1.; j += 1.)
            result += texture(shadowtex, vec4(shadowtexcoord + vec2(i,j) / shadow_res, float(index), d));
    }

    return result / 9.;
}

void main() {
    vec2 uv = gl_FragCoord.xy / screen;
    float z = texture(dtex, uv).x;
    vec4 xpos = getPosFromUVDepth(vec3(uv, z), InverseProjectionMatrix);

    vec3 norm = normalize(DecodeNormal(2. * texture(ntex, uv).xy - 1.));
    float roughness =texture(ntex, uv).z;
    vec3 eyedir = -normalize(xpos.xyz);

    vec3 Lightdir = SunMRP(norm, eyedir);
    float NdotL = clamp(dot(norm, Lightdir), 0., 1.);
    vec3 color = texture(ctex, uv).rgb;

    float metalness = texture(ntex, uv).a;

    // Shadows
    float factor;
    if (xpos.z < split0)
        factor = getShadowFactor(xpos.xyz, 0);
    else if (xpos.z < split1)
        factor = getShadowFactor(xpos.xyz, 1);
    else if (xpos.z < split2)
        factor = getShadowFactor(xpos.xyz, 2);
    else if (xpos.z < splitmax)
        factor = getShadowFactor(xpos.xyz, 3);
    else
        factor = 1.;

    vec3 Dielectric = DiffuseBRDF(norm, eyedir, Lightdir, color, roughness) + SpecularBRDF(norm, eyedir, Lightdir, vec3(.04), roughness);
    vec3 Metal = SpecularBRDF(norm, eyedir, Lightdir, color, roughness);
    FragColor = vec4(factor * NdotL * sun_col * mix(Dielectric, Metal, metalness), 0.);
}
