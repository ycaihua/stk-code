uniform sampler2D ntex;
uniform sampler2D ctex;
uniform sampler2D dtex;

out vec4 FragColor;

vec3 DecodeNormal(vec2 n);
vec3 SpecularBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness);
vec3 DiffuseBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness);
vec4 getPosFromUVDepth(vec3 uvDepth, mat4 InverseProjectionMatrix);
vec3 SunMRP(vec3 normal, vec3 eyedir);

void main() {
    vec2 uv = gl_FragCoord.xy / screen;
    float z = texture(dtex, uv).x;
    vec4 xpos = getPosFromUVDepth(vec3(uv, z), InverseProjectionMatrix);

    vec3 norm = normalize(DecodeNormal(2. * texture(ntex, uv).xy - 1.));
    vec3 color = texture(ctex, uv).xyz;
    float roughness = texture(ntex, uv).z;
    vec3 eyedir = -normalize(xpos.xyz);

    vec3 Lightdir = SunMRP(norm, eyedir);
    float NdotL = clamp(dot(norm, Lightdir), 0., 1.);

    float reflectance = texture(ntex, uv).a;

/*	if (hasclouds == 1)
	{
		vec2 cloudcoord = (xpos.xz * 0.00833333) + wind;
		float cloud = texture(cloudtex, cloudcoord).x;
		//float cloud = step(0.5, cloudcoord.x) * step(0.5, cloudcoord.y);

		outcol *= cloud;
	}*/


    FragColor = vec4(NdotL * sun_col * mix(DiffuseBRDF(norm, eyedir, Lightdir, color, roughness), SpecularBRDF(norm, eyedir, Lightdir, color, roughness), reflectance), 0.);
}
