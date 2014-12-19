uniform sampler2D ctex;
uniform sampler2D dtex;
uniform sampler2D matte;
uniform float strength;

out vec4 FragColor;

vec4 getPosFromUVDepth(vec3 uvDepth, mat4 InverseProjectionMatrix);
void main()
{
    vec2 uv = gl_FragCoord.xy / screen;
    float X = uv.x;
    float Y = uv.y;
    float z = texture(dtex, uv).x;

    vec3 pos = getPosFromUVDepth(vec3(uv, z), InverseProjectionMatrix).xyz;
    float depth = pos.z;
    float stepsize = max(0., strength * 11. / (depth / 100. + 800. * abs(dFdy(depth / 100.)))) / screen.x;

    // We use sampling from Jorge Jimenez
    float w[3] = { 0.242,  0.061, 0.006 };
    float o[3] = { 0.3333, 0.6667, 1.0 };

    vec4 sum = texture(ctex, vec2(X, Y)) * 0.382;

    float total_weight = 0.382;
    for (int i = 0; i < 3; i++) {
        vec2 uv_m = vec2(X - o[i] * stepsize, Y);
        float z_m = texture(dtex, uv_m).x;
        float depth_m = getPosFromUVDepth(vec3(uv_m, z_m), InverseProjectionMatrix).z;
        float tmp = max(0., 1. - .1 * abs(depth_m - pos.z)) * texture(matte, uv_m).r;
        sum += texture(ctex, uv_m) * w[i] * tmp;
        total_weight += w[i] * tmp;

        vec2 uv_p = vec2(X + o[i] * stepsize, Y);
        float z_p = texture(dtex, uv_p).x;
        float depth_p = getPosFromUVDepth(vec3(uv_p, z_p), InverseProjectionMatrix).z;
        tmp = max(0., 1. - .1 * abs(depth_p - pos.z)) * texture(matte, uv_p).r;
        sum += texture(ctex, uv_p) * w[i] * tmp;
        total_weight += w[i] * tmp;
    }

    FragColor = sum / total_weight;
}

