uniform sampler2D ntex;
uniform sampler2D dtex;
uniform sampler2D ctex;
uniform sampler2DArray shadowtex;

uniform float split0;
uniform float split1;
uniform float split2;
uniform float splitmax;

in vec2 uv;

// We need 2 outputs since specular is applied after subsurface
layout(location = 0) out vec3 Diffuse;
layout(location = 1) out vec3 Specular;

vec3 DecodeNormal(vec2 n);
vec3 SpecularBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness);
vec3 DiffuseBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness);
vec4 getPosFromUVDepth(vec3 uvDepth, mat4 InverseProjectionMatrix);
vec3 SunMRP(vec3 normal, vec3 eyedir);
vec3 DiffuseIBL(vec3 normal, vec3 V, float roughness, vec3 color);
vec3 SpecularIBL(vec3 normal, vec3 V, float roughness, vec3 F0);

float getShadowFactor(vec3 pos, int index)
{
    vec4 shadowcoord = (ShadowViewProjMatrixes[index] * InverseViewMatrix * vec4(pos, 1.0));
    shadowcoord.xy /= shadowcoord.w;
    vec2 shadowtexcoord = shadowcoord.xy * 0.5 + 0.5;

    float z = texture(shadowtex, vec3(shadowtexcoord, float(index))).x;
    float d = shadowcoord.z;
    return min(exp(-32. * d) * z, 1.);
}

vec3 Transmittance(float s)
{
    // It's skin transmittance profile from
    // http://www.iryoku.com/translucency/downloads/Real-Time-Realistic-Skin-Translucency.pdf
    // But with r and b channel swapped
    vec3 val = vec3(.649, .455, .233) * exp(- s * s / .0064) +
        vec3(.344, .336, .1) * exp(- s * s / .0484) +
        vec3(0., .198, .118) * exp(- s * s / .187) +
        vec3(.007, .007, .113) * exp(- s * s / .567) +
        vec3(.0, .004, .358) * exp(- s * s / 1.99) +
        vec3(.0, .0, .078) * exp(- s * s / 7.41);
    return val;
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
    xpos.xyz -= norm * .0005;

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

    float d = - log(factor) / 32.;
    // We use lambert because roughness doesnt matter in transmitance
    vec3 other =  sun_col * max(0., .3 + dot(-norm, Lightdir)) * color * Transmittance(100. * d);

    Diffuse = pow(factor, 8.) * sun_col * NdotL *  DiffuseBRDF(norm, eyedir, Lightdir, color, roughness) + .2 * DiffuseIBL(norm, eyedir, roughness, color) + other;
    Specular = pow(factor, 8.) * sun_col * NdotL * SpecularBRDF(norm, eyedir, Lightdir, vec3(.04), roughness) + .2 * SpecularIBL(norm, eyedir, roughness, vec3(0.04));
}
