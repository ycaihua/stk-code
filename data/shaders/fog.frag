uniform sampler2D tex;
uniform sampler2DArray shadowtex;

uniform float density;
uniform vec3 col;

out vec4 Fog;

float getShadowFactor(vec3 pos)
{
    int index;
    vec4 shadowcoord;
    vec2 shadowtexcoord;
    for (index = 0; index < 4; index++)
    {
        shadowcoord = (ShadowViewProjMatrixes[index] * InverseViewMatrix * vec4(pos, 1.0));
        shadowcoord.xy /= shadowcoord.w;
        shadowtexcoord = shadowcoord.xy * 0.5 + 0.5;
        if (shadowtexcoord.x >= 0. && shadowtexcoord.x <= 1. && shadowtexcoord.y >= 0. && shadowtexcoord.y <= 1.)
            break;
    }

    float z = texture(shadowtex, vec3(shadowtexcoord, float(index))).x;
    float d = shadowcoord.z;
    return min(pow(exp(-8. * d) * z, 256.), 1.);
}

vec4 getPosFromUVDepth(vec3 uvDepth, mat4 InverseProjectionMatrix);

void main()
{
    vec2 uv = 2. * gl_FragCoord.xy / screen;
    float z = min(texture(tex, uv).x, 0.999);

    vec4 xpos = getPosFromUVDepth(vec3(uv, z), InverseProjectionMatrix);
    vec3 eyedir = -normalize(xpos.xyz);

    float stepsize = length(xpos.xyz) / 128;
    vec3 fog = vec3(0.);
    // Normalized on the cpu
//    vec3 L = direction;

    for (int i = 0; i < 128; i++)
    {
        float l = (128 - i) * stepsize;
        fog += .05 * col * getShadowFactor(xpos.xyz) * exp(- .05 * l) * stepsize;
        xpos.xyz += stepsize * eyedir;
    }

    Fog = vec4(fog, 1.);
}
