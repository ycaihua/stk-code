uniform sampler2D tex;
uniform float vignette_weight;

out vec4 FragColor;

vec3 getCIEYxy(vec3 rgbColor);
vec3 getRGBFromCIEXxy(vec3 YxyColor);

void main()
{
    vec2 uv = gl_FragCoord.xy / screen;
    vec4 col = texture(tex, uv);

    vec3 Yxy = getCIEYxy(col.rgb);
    col.rgb = getRGBFromCIEXxy(vec3(1.5 * Yxy.x, Yxy.y, Yxy.z));

    // Uncharted2 tonemap with Auria's custom coefficients
    vec4 perChannel = (col * (6.9 * col + .6)) / (col * (5.2 * col + 2.5) + 0.06);
    perChannel = pow(perChannel, vec4(2.2));

    vec2 inside = uv - 0.5;
    float vignette = 1. - dot(inside, inside) * vignette_weight;
    vignette = clamp(pow(vignette, 0.8), 0., 1.);

    FragColor = vec4(perChannel.xyz * vignette, col.a);
}
