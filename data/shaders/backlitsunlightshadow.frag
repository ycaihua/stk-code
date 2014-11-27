uniform sampler2D ntex;
uniform sampler2D dtex;
uniform sampler2D ctex;
uniform sampler2DArray shadowtex;

uniform float split0;
uniform float split1;
uniform float split2;
uniform float splitmax;

in vec2 uv;
out vec4 FragColor;

vec3 DecodeNormal(vec2 n);
vec3 SpecularBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness);
vec3 DiffuseBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness);
vec4 getPosFromUVDepth(vec3 uvDepth, mat4 InverseProjectionMatrix);
vec3 SunMRP(vec3 normal, vec3 eyedir);

float getShadowFactor(vec3 pos, int index)
{
    vec4 shadowcoord = (ShadowViewProjMatrixes[index] * InverseViewMatrix * vec4(pos, 1.0));
    shadowcoord.xy /= shadowcoord.w;
    vec2 shadowtexcoord = shadowcoord.xy * 0.5 + 0.5;

    float z = texture(shadowtex, vec3(shadowtexcoord, float(index))).x;
    float d = shadowcoord.z - 0.005;
    return min(pow(exp(-32. * d) * z, 8.), 1.);
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

    // Inspired from http://http.developer.nvidia.com/GPUGems3/gpugems3_ch16.html
    float fEdotL = clamp(dot(Lightdir, eyedir), 0., 1.);
    float fPowEdotL = pow(fEdotL, 4.);

    float backNdotL = clamp(dot(-norm, Lightdir), 0., 1.);

    vec3 Dielectric = NdotL * DiffuseBRDF(norm, eyedir, Lightdir, color, roughness) + NdotL * SpecularBRDF(norm, eyedir, Lightdir, vec3(.04), roughness) + mix(backNdotL, fPowEdotL, dot(color, vec3(1.)) / 3.) * DiffuseBRDF(-norm, eyedir, Lightdir, color, roughness);
    vec3 Metal = NdotL * SpecularBRDF(norm, eyedir, Lightdir, color, roughness);
    FragColor = vec4(factor * sun_col * mix(Dielectric, Metal, metalness), 0.);
}
