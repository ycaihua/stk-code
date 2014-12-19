uniform sampler2D subsurface_layer0;
uniform sampler2D subsurface_layer1;
uniform sampler2D specular;

out vec4 FragColor;

void main()
{
    vec2 uv = gl_FragCoord.xy / screen;
    vec3 col0 = texture(subsurface_layer0, uv).rgb;
    vec3 col1 = texture(subsurface_layer1, uv).rgb;
    vec3 finalcolor = col0 * vec3(.9, .5, .3) + col1 * vec3(.1, .5, .7) + texture(specular, uv).rgb;
    FragColor = vec4(finalcolor, 1.);
}
