#ifdef Use_Bindless_Texture
layout(bindless_sampler) uniform sampler2D tex_layout;
layout(bindless_sampler) uniform sampler2D tex_detail0;
layout(bindless_sampler) uniform sampler2D tex_detail1;
layout(bindless_sampler) uniform sampler2D tex_detail2;
layout(bindless_sampler) uniform sampler2D tex_detail3;
layout(bindless_sampler) uniform sampler2D glosstex;
#else
uniform sampler2D tex_layout;
uniform sampler2D tex_detail0;
uniform sampler2D tex_detail1;
uniform sampler2D tex_detail2;
uniform sampler2D tex_detail3;
uniform sampler2D glosstex;
#endif

in vec3 nor;
in vec2 uv;
in vec2 uv_bis;

layout(location = 0) out vec4 EncodedNormal_Roughness_Metalness;
layout(location = 1) out vec4 Colors;
layout(location = 2) out float EmitMap;

vec2 EncodeNormal(vec3 n);

void main() {
    // Splatting part
    vec4 splatting = texture(tex_layout, uv_bis);
    vec4 detail0 = texture(tex_detail0, uv);
    vec4 detail1 = texture(tex_detail1, uv);
    vec4 detail2 = texture(tex_detail2, uv);
    vec4 detail3 = texture(tex_detail3, uv);
    vec4 detail4 = vec4(0.0);
#ifdef Use_Bindless_Texture
#ifdef SRGBBindlessFix
    detail0.xyz = pow(detail0.xyz, vec3(2.2));
    detail1.xyz = pow(detail1.xyz, vec3(2.2));
    detail2.xyz = pow(detail2.xyz, vec3(2.2));
    detail3.xyz = pow(detail3.xyz, vec3(2.2));
#endif
#endif

    vec4 splatted = splatting.r * detail0 +
        splatting.g * detail1 +
        splatting.b * detail2 +
        max(0., (1.0 - splatting.r - splatting.g - splatting.b)) * detail3;

    Colors = vec4(splatted.rgb, 1.);

    float glossmap = texture(glosstex, uv).r;
    float reflectance = texture(glosstex, uv).g;
    EncodedNormal_Roughness_Metalness.xy = 0.5 * EncodeNormal(normalize(nor)) + 0.5;
    EncodedNormal_Roughness_Metalness.z = 1. - glossmap;
    EncodedNormal_Roughness_Metalness.w = reflectance;
    EmitMap = texture(glosstex, uv).b;
}
