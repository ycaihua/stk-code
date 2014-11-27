#ifdef Use_Bindless_Texture
layout(bindless_sampler) uniform sampler2D Albedo;
layout(bindless_sampler) uniform sampler2D Detail;
layout(bindless_sampler) uniform sampler2D SpecMap;
#else
uniform sampler2D Albedo;
uniform sampler2D Detail;
uniform sampler2D SpecMap;
#endif

in vec3 nor;
in vec2 uv;
in vec2 uv_bis;

layout(location = 0) out vec4 EncodedNormal_Specular_Reflectance;
layout(location = 1) out vec4 Colors;
layout(location = 2) out float EmitMap;

vec2 EncodeNormal(vec3 n);

void main(void)
{
    vec4 color = texture(Albedo, uv);
#ifdef Use_Bindless_Texture
#ifdef SRGBBindlessFix
    color.xyz = pow(color.xyz, vec3(2.2));
#endif
#endif
    vec4 detail = texture(Detail, uv_bis);
    color *= detail;
    float glossmap = texture(SpecMap, uv).r;
    float reflectance = texture(SpecMap, uv).g;

    EncodedNormal_Specular_Reflectance.xy = 0.5 * EncodeNormal(normalize(nor)) + 0.5;
    EncodedNormal_Specular_Reflectance.z = glossmap;
    EncodedNormal_Specular_Reflectance.w = reflectance;
    Colors = vec4(color.rgb, 1.);
    EmitMap = texture(SpecMap, uv).b;
}
