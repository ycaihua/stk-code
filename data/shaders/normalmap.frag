#ifdef Use_Bindless_Texture
layout(bindless_sampler) uniform sampler2D normalMap;
layout(bindless_sampler) uniform sampler2D DiffuseForAlpha;
layout(bindless_sampler) uniform sampler2D tex;
#else
uniform sampler2D normalMap;
uniform sampler2D DiffuseForAlpha;
uniform sampler2D tex;
#endif

in vec3 tangent;
in vec3 bitangent;
in vec2 uv;

layout(location = 0) out vec4 EncodedNormal_Roughness_Metalness;
layout(location = 1) out vec4 Colors;
layout(location = 2) out float EmitMap;

vec2 EncodeNormal(vec3 n);

void main()
{
    // normal in Tangent Space
    vec3 TS_normal = 2.0 * texture(normalMap, uv).rgb - 1.0;
    float gloss = texture(DiffuseForAlpha, uv).r;
    float reflectance = texture(DiffuseForAlpha, uv).g;
    // Because of interpolation, we need to renormalize
    vec3 Frag_tangent = normalize(tangent);
    vec3 Frag_normal = normalize(cross(Frag_tangent, bitangent));
    vec3 Frag_bitangent = cross(Frag_normal, Frag_tangent);

    vec3 FragmentNormal = TS_normal.x * Frag_tangent + TS_normal.y * Frag_bitangent - TS_normal.z * Frag_normal;
    EncodedNormal_Roughness_Metalness.xy = 0.5 * EncodeNormal(normalize(FragmentNormal)) + 0.5;
    EncodedNormal_Roughness_Metalness.z = 1. - gloss;
    EncodedNormal_Roughness_Metalness.w = reflectance;

    vec4 col = texture(tex, uv);
#ifdef Use_Bindless_Texture
#ifdef SRGBBindlessFix
    col.xyz = pow(col.xyz, vec3(2.2));
#endif
#endif
    Colors = vec4(col.rgb, 1.);
    EmitMap = texture(DiffuseForAlpha, uv).b;
}
