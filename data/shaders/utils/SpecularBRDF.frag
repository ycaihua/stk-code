// Fresnel Schlick approximation
vec3 Fresnel(vec3 viewdir, vec3 halfdir, vec3 basecolor)
{
    return clamp(basecolor + (1. - basecolor) * pow(1. - clamp(dot(viewdir, halfdir), 0., 1.), 5.), vec3(0.), vec3(1.));
}

// Reduced because we include the term below the fraction
// We use implicit
vec3 ReducedGeometric()
{
    return vec3(1.);
}

// Blinn Phong
float Distribution(float roughness, vec3 normal, vec3 halfdir)
{
    float r = exp2(10. * roughness + 1.);
    float NdotH = clamp(dot(normal, halfdir), 0., 1.);
    float normalisationFactor = (r + 2.) / (2. * 3.14);
    return normalisationFactor * pow(NdotH, r);
}

vec3 SpecularBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness)
{
    // Half Light View direction
    vec3 H = normalize(eyedir + lightdir);

    // Microfacet model
    return Fresnel(eyedir, H, color) * ReducedGeometric() * Distribution(roughness, normal, H) / 4.;
}