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

layout(location = 0) out vec4 EncodedNormal_Specular_Reflectance;
layout(location = 1) out vec4 Colors;
layout(location = 2) out float EmitMap;

vec2 EncodeNormal(vec3 n);

void main(void)
{
#ifdef Use_Bindless_Texture
    vec4 col = texture(handle, uv);
#ifdef SRGBBindlessFix
    col.xyz = pow(col.xyz, vec3(2.2));
#endif
    Colors = vec4(col.xyz * pow(color.rgb, vec3(2.2)), 1.);
    float glossmap = texture(secondhandle, uv).x;
    float reflectance = texture(secondhandle, uv).g;
    float emitval = texture(secondhandle, uv).b;
#else
    Colors = vec4(texture(tex, uv).xyz * pow(color.rgb, vec3(2.2)), 1.);
    float glossmap = texture(glosstex, uv).x;
    float reflectance = texture(glosstex, uv).g;
    float emitval = texture(glosstex, uv).b;
#endif
    EncodedNormal_Specular_Reflectance.xy = 0.5 * EncodeNormal(normalize(nor)) + 0.5;
    EncodedNormal_Specular_Reflectance.z = glossmap;
    EncodedNormal_Specular_Reflectance.w = reflectance;
    EmitMap = emitval;
}
