#ifndef Use_Bindless_Texture
uniform sampler2D tex;
uniform sampler2D glosstex;
#endif

#ifdef Use_Bindless_Texture
flat in sampler2D handle;
flat in sampler2D secondhandle;
#endif
in vec4 color;
in vec3 nor;
in vec2 uv;

layout(location = 0) out vec4 EncodedNormal_Roughness_Metalness;
layout(location = 1) out vec4 Colors;
layout(location = 2) out float EmitMap;

vec2 EncodeNormal(vec3 n);

void main() {
#ifdef Use_Bindless_Texture
    vec4 col = texture(handle, uv);
    float glossmap = texture(secondhandle, uv).x;
    float reflectance = texture(secondhandle, uv).g;
    float emitval = texture(secondhandle, uv).b;
#ifdef SRGBBindlessFix
    col.xyz = pow(col.xyz, vec3(2.2));
#endif
#else
    vec4 col = texture(tex, uv);
    float glossmap = texture(glosstex, uv).x;
    float reflectance = texture(glosstex, uv).g;
    float emitval = texture(glosstex, uv).b;
#endif
    if (col.a < 0.5)
        discard;
    Colors = vec4(col.rgb * pow(color.rgb, vec3(2.2)), 1.);
    EncodedNormal_Roughness_Metalness.xy = 0.5 * EncodeNormal(normalize(nor)) + 0.5;
    EncodedNormal_Roughness_Metalness.z = 1. - glossmap;
    EncodedNormal_Roughness_Metalness.w = reflectance;
    EmitMap = emitval;
}

