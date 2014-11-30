#ifndef Use_Bindless_Texture
uniform sampler2D Albedo;
uniform sampler2D Detail;
uniform sampler2D SpecMap;
#endif

#ifdef Use_Bindless_Texture
flat in sampler2D handle;
flat in sampler2D secondhandle;
flat in sampler2D thirdhandle;
#endif
in vec3 nor;
in vec2 uv;
in vec2 uv_bis;

layout(location = 0) out vec4 EncodedNormal_Roughness_Metalness;
layout(location = 1) out vec4 Colors;
layout(location = 2) out float EmitMap;

vec2 EncodeNormal(vec3 n);

void main(void)
{
#ifdef Use_Bindless_Texture
    vec4 color = texture(handle, uv);
    float glossmap = texture(secondhandle, uv).r;
    float reflectance = texture(secondhandle, uv).g;
    float emitval = texture(secondhandle, uv).b;
#ifdef SRGBBindlessFix
    color.xyz = pow(color.xyz, vec3(2.2));
#endif
    vec4 detail = texture(thirdhandle, uv_bis);
#else
    vec4 color = texture(Albedo, uv);
    vec4 detail = texture(Detail, uv_bis);
    float glossmap = texture(SpecMap, uv).r;
    float reflectance = texture(SpecMap, uv).g;
    float emitval = texture(SpecMap, uv).b;
#endif
    color *= detail;

    EncodedNormal_Roughness_Metalness.xy = 0.5 * EncodeNormal(normalize(nor)) + 0.5;
    EncodedNormal_Roughness_Metalness.z = 1. - glossmap;
    EncodedNormal_Roughness_Metalness.w = reflectance;
    Colors = vec4(color.rgb, 1.);
    EmitMap = emitval;
}
