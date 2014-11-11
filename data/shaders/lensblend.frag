uniform sampler2D tex_128;
uniform sampler2D tex_256;
uniform sampler2D tex_512;

out vec4 FragColor;

void main()
{
    vec2 uv = gl_FragCoord.xy / screen;
    vec4 col = texture(tex_128, uv);
    
    float final = max(col.r,max(col.g,col.b));
    //final = final * 2;
    vec3 blue = vec3(final * 0.1, final * 0.2, final);
    
    FragColor = vec4(blue, 1.);
}
