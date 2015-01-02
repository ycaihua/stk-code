#define MAX_JOINT_NUM 48

uniform mat4 ModelMatrix =
    mat4(1., 0., 0., 0.,
         0., 1., 0., 0.,
         0., 0., 1., 0.,
         0., 0., 0., 1.);
uniform mat4 InverseModelMatrix =
    mat4(1., 0., 0., 0.,
         0., 1., 0., 0.,
         0., 0., 1., 0.,
         0., 0., 0., 1.);

uniform mat4 TextureMatrix =
    mat4(1., 0., 0., 0.,
         0., 1., 0., 0.,
         0., 0., 1., 0.,
         0., 0., 0., 1.);

#if __VERSION__ >= 330
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec4 Color;
layout(location = 3) in vec2 Texcoord;
layout(location = 4) in vec2 SecondTexcoord;
layout(location = 5) in vec3 Tangent;
layout(location = 6) in vec3 Bitangent;

layout(location = 7) in int index0;
layout(location = 8) in float weight0;
layout(location = 9) in int index1;
layout(location = 10) in float weight1;
layout(location = 11) in int index2;
layout(location = 12) in float weight2;
layout(location = 13) in int index3;
layout(location = 14) in float weight3;
#else
in vec3 Position;
in vec3 Normal;
in vec4 Color;
in vec2 Texcoord;
in vec2 SecondTexcoord;
in vec3 Tangent;
in vec3 Bitangent;
#endif

out vec3 nor;
out vec3 tangent;
out vec3 bitangent;
out vec2 uv;
out vec2 uv_bis;
out vec4 color;

uniform mat4 JointTransform[MAX_JOINT_NUM];

void main()
{
    vec4 IdlePosition = vec4(Position, 1.);
    vec4 SkinnedPosition = vec4(0.);

    if (index0 >= 0)
    {
        vec4 tmp = JointTransform[index0] * IdlePosition;
        SkinnedPosition += weight0 * tmp / tmp.w;
    }

    if (index1 >= 0)
    {
        vec4 tmp = JointTransform[index1] * IdlePosition;
        SkinnedPosition += weight1 * tmp / tmp.w;
    }

    if (index2 >= 0)
    {
        vec4 tmp = JointTransform[index2] * IdlePosition;
        SkinnedPosition += weight2 * tmp / tmp.w;
    }

    if (index3 >= 0)
    {
        vec4 tmp = JointTransform[index3] * IdlePosition;
        SkinnedPosition += weight3 * tmp / tmp.w;
    }

    color = Color.zyxw;
    mat4 ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix;
    mat4 TransposeInverseModelView = transpose(InverseModelMatrix * InverseViewMatrix);
    gl_Position = ModelViewProjectionMatrix * vec4(SkinnedPosition.xyz, 1.);
    nor = (TransposeInverseModelView * vec4(Normal, 0.)).xyz;
    tangent = (TransposeInverseModelView * vec4(Tangent, 1.)).xyz;
    bitangent = (TransposeInverseModelView * vec4(Bitangent, 1.)).xyz;
    uv = (TextureMatrix * vec4(Texcoord, 1., 1.)).xy;
    uv_bis = SecondTexcoord;
}
