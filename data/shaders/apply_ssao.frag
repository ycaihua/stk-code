uniform sampler2D ssao;

out vec4 FragColor;

void main()
{
    vec2 uv = gl_FragCoord.xy / screen;
    FragColor = vec4(texture(ssao, uv).rrr, 1.);
}
