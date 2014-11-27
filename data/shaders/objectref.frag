#ifdef Use_Bindless_Texture
layout(bindless_sampler) uniform sampler2D tex;
layout(bindless_sampler) uniform sampler2D glosstex;
#else
uniform sampler2D tex;
uniform sampler2D glosstex;
#endif

in vec4 color;
in vec3 nor;
in vec2 uv;

layout(location = 0) out vec4 EncodedNormal_Specular_Reflectance;
layout(location = 1) out vec4 Colors;
layout(location = 2) out float EmitMap;

vec2 EncodeNormal(vec3 n);

void main() {
    vec4 col = texture(tex, uv);
    if (col.a < 0.5)
        discard;
    Colors = vec4(col.rgb * pow(color.rgb, vec3(2.2)), 1.);
    float glossmap = texture(glosstex, uv).r;
    float reflectance = texture(glosstex, uv).g;
    EncodedNormal_Specular_Reflectance.xy = 0.5 * EncodeNormal(normalize(nor)) + 0.5;
    EncodedNormal_Specular_Reflectance.z = glossmap;
    EncodedNormal_Specular_Reflectance.w = reflectance;
    EmitMap = texture(glosstex, uv).b;
}

