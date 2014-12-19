#include "graphics/irr_driver.hpp"
#include "central_settings.hpp"
#include "config/user_config.hpp"
#include "graphics/callbacks.hpp"
#include "graphics/glwrap.hpp"
#include "graphics/post_processing.hpp"
#include "graphics/rtts.hpp"
#include "graphics/shaders.hpp"
#include "graphics/stkanimatedmesh.hpp"
#include "modes/world.hpp"
#include "utils/log.hpp"
#include "utils/profiler.hpp"
#include "utils/tuple.hpp"
#include "stkscenemanager.hpp"
#include "utils/profiler.hpp"

#include <S3DVertex.h>

/**
\page render_geometry Geometry Rendering Overview

\section adding_material Adding a solid material

You need to consider twice before adding a new material : in the worst case a material requires 8 shaders :
one for each solid pass, one for shadow pass, one for RSM pass, and you need to double that for instanced version.

You need to declare a new enum in MeshMaterial and to write the corresponding dispatch code in MaterialTypeToMeshMaterial
and to create 2 new List* structures (one for standard and one for instanced version).

Then you need to write the code in stkscenemanager.cpp that will add any mesh with the new material to their corresponding
lists : in handleSTKCommon for the standard version and in the body of PrepareDrawCalls for instanced version.

\section vertex_layout Available Vertex Layout

There are 3 different layout that comes from Irrlicht loading routines :
EVT_STANDARD, EVT_2TCOORDS, EVT_TANGENT.

Below are the attributes for each vertex layout and their predefined location.

\subsection EVT_STANDARD
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec4 Color;
layout(location = 3) in vec2 Texcoord;

\subsection EVT_2TCOORDS
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec4 Color;
layout(location = 3) in vec2 Texcoord;
layout(location = 4) in vec2 SecondTexcoord;

\subsection EVT_TANGENT
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec4 Color;
layout(location = 3) in vec2 Texcoord;
layout(location = 5) in vec3 Tangent;
layout(location = 6) in vec3 Bitangent;

*/

struct DefaultMaterial
{
    typedef MeshShader::InstancedObjectShader InstancedFirstPassShader;
    typedef MeshShader::InstancedShadowShader InstancedShadowPassShader;
    typedef MeshShader::InstancedRSMShader InstancedRSMShader;
    typedef ListInstancedMatDefault InstancedList;
    typedef MeshShader::ObjectShader FirstPassShader;
    typedef MeshShader::ShadowShader ShadowPassShader;
    typedef MeshShader::RSMShader RSMShader;
    typedef ListMatDefault List;
    static const enum video::E_VERTEX_TYPE VertexType = video::EVT_STANDARD;
    static const enum Material::ShaderType MaterialType = Material::SHADERTYPE_SOLID;
    static const enum InstanceType Instance = InstanceTypeDualTex;
    static const STK::Tuple<size_t, size_t> FirstPassTextures;
    static const STK::Tuple<> ShadowTextures;
    static const STK::Tuple<size_t> RSMTextures;
};

const STK::Tuple<size_t, size_t> DefaultMaterial::FirstPassTextures = STK::Tuple<size_t, size_t>(0, 1);
const STK::Tuple<> DefaultMaterial::ShadowTextures;
const STK::Tuple<size_t> DefaultMaterial::RSMTextures = STK::Tuple<size_t>(0);

struct AlphaRef
{
    typedef MeshShader::InstancedObjectRefShader InstancedFirstPassShader;
    typedef MeshShader::InstancedRefShadowShader InstancedShadowPassShader;
    typedef MeshShader::InstancedRSMShader InstancedRSMShader;
    typedef ListInstancedMatAlphaRef InstancedList;
    typedef MeshShader::ObjectRefShader FirstPassShader;
    typedef MeshShader::RefShadowShader ShadowPassShader;
    typedef MeshShader::RSMShader RSMShader;
    typedef ListMatAlphaRef List;
    static const enum video::E_VERTEX_TYPE VertexType = video::EVT_STANDARD;
    static const enum Material::ShaderType MaterialType = Material::SHADERTYPE_ALPHA_TEST;
    static const enum InstanceType Instance = InstanceTypeDualTex;
    static const STK::Tuple<size_t, size_t> FirstPassTextures;
    static const STK::Tuple<size_t> ShadowTextures;
    static const STK::Tuple<size_t> RSMTextures;
};

const STK::Tuple<size_t, size_t> AlphaRef::FirstPassTextures = STK::Tuple<size_t, size_t>(0, 1);
const STK::Tuple<size_t> AlphaRef::ShadowTextures = STK::Tuple<size_t>(0);
const STK::Tuple<size_t> AlphaRef::RSMTextures = STK::Tuple<size_t>(0);

struct UnlitMat
{
    typedef MeshShader::InstancedObjectRefShader InstancedFirstPassShader;
    typedef MeshShader::InstancedRefShadowShader InstancedShadowPassShader;
    typedef MeshShader::InstancedRSMShader InstancedRSMShader;
    typedef ListInstancedMatUnlit InstancedList;
    typedef MeshShader::ObjectRefShader FirstPassShader;
    typedef MeshShader::RefShadowShader ShadowPassShader;
    typedef MeshShader::RSMShader RSMShader;
    typedef ListMatUnlit List;
    static const enum video::E_VERTEX_TYPE VertexType = video::EVT_STANDARD;
    static const enum Material::ShaderType MaterialType = Material::SHADERTYPE_SOLID_UNLIT;
    static const enum InstanceType Instance = InstanceTypeDualTex;
    static const STK::Tuple<size_t, size_t> FirstPassTextures;
    static const STK::Tuple<size_t> ShadowTextures;
    static const STK::Tuple<size_t> RSMTextures;
};

const STK::Tuple<size_t, size_t> UnlitMat::FirstPassTextures = STK::Tuple<size_t, size_t>(0, 1);
const STK::Tuple<size_t> UnlitMat::ShadowTextures = STK::Tuple<size_t>(0);
const STK::Tuple<size_t> UnlitMat::RSMTextures = STK::Tuple<size_t>(0);

struct GrassMat
{
    typedef MeshShader::InstancedGrassShader InstancedFirstPassShader;
    typedef MeshShader::InstancedGrassShadowShader InstancedShadowPassShader;
    typedef MeshShader::InstancedRSMShader InstancedRSMShader;
    typedef ListInstancedMatGrass InstancedList;
    typedef MeshShader::GrassShader FirstPassShader;
    typedef MeshShader::GrassShadowShader ShadowPassShader;
    typedef MeshShader::RSMShader RSMShader;
    typedef ListMatGrass List;
    static const enum video::E_VERTEX_TYPE VertexType = video::EVT_STANDARD;
    static const enum Material::ShaderType MaterialType = Material::SHADERTYPE_VEGETATION;
    static const enum InstanceType Instance = InstanceTypeDualTex;
    static const STK::Tuple<size_t, size_t> FirstPassTextures;
    static const STK::Tuple<size_t> ShadowTextures;
    static const STK::Tuple<size_t> RSMTextures;
};

const STK::Tuple<size_t, size_t> GrassMat::FirstPassTextures = STK::Tuple<size_t, size_t>(0, 1);
const STK::Tuple<size_t> GrassMat::ShadowTextures = STK::Tuple<size_t>(0);
const STK::Tuple<size_t> GrassMat::RSMTextures = STK::Tuple<size_t>(0);

struct NormalMat
{
    typedef MeshShader::InstancedNormalMapShader InstancedFirstPassShader;
    typedef MeshShader::InstancedShadowShader InstancedShadowPassShader;
    typedef MeshShader::InstancedRSMShader InstancedRSMShader;
    typedef ListInstancedMatNormalMap InstancedList;
    typedef MeshShader::NormalMapShader FirstPassShader;
    typedef MeshShader::ShadowShader ShadowPassShader;
    typedef MeshShader::RSMShader RSMShader;
    typedef ListMatNormalMap List;
    static const enum video::E_VERTEX_TYPE VertexType = video::EVT_TANGENTS;
    static const enum Material::ShaderType MaterialType = Material::SHADERTYPE_NORMAL_MAP;
    static const enum InstanceType Instance = InstanceTypeThreeTex;
    static const STK::Tuple<size_t, size_t, size_t> FirstPassTextures;
    static const STK::Tuple<> ShadowTextures;
    static const STK::Tuple<size_t> RSMTextures;
};

const STK::Tuple<size_t, size_t, size_t> NormalMat::FirstPassTextures = STK::Tuple<size_t, size_t, size_t>(2, 1, 0);
const STK::Tuple<> NormalMat::ShadowTextures;
const STK::Tuple<size_t> NormalMat::RSMTextures = STK::Tuple<size_t>(0);

struct DetailMat
{
    typedef MeshShader::InstancedDetailledObjectShader InstancedFirstPassShader;
    typedef MeshShader::InstancedShadowShader InstancedShadowPassShader;
    typedef MeshShader::InstancedRSMShader InstancedRSMShader;
    typedef ListInstancedMatDetails InstancedList;
    typedef MeshShader::DetailledObjectShader FirstPassShader;
    typedef MeshShader::ShadowShader ShadowPassShader;
    typedef MeshShader::RSMShader RSMShader;
    typedef ListMatDetails List;
    static const enum video::E_VERTEX_TYPE VertexType = video::EVT_2TCOORDS;
    static const enum Material::ShaderType MaterialType = Material::SHADERTYPE_DETAIL_MAP;
    static const enum InstanceType Instance = InstanceTypeThreeTex;
    static const STK::Tuple<size_t, size_t, size_t> FirstPassTextures;
    static const STK::Tuple<> ShadowTextures;
    static const STK::Tuple<size_t> RSMTextures;
};

const STK::Tuple<size_t, size_t, size_t> DetailMat::FirstPassTextures = STK::Tuple<size_t, size_t, size_t>(0, 2, 1);
const STK::Tuple<> DetailMat::ShadowTextures;
const STK::Tuple<size_t> DetailMat::RSMTextures = STK::Tuple<size_t>(0);

struct SplattingMat
{
    typedef MeshShader::SplattingShader FirstPassShader;
    typedef MeshShader::ShadowShader ShadowPassShader;
    typedef MeshShader::SplattingRSMShader RSMShader;
    typedef ListMatSplatting List;
    static const enum video::E_VERTEX_TYPE VertexType = video::EVT_2TCOORDS;
    static const STK::Tuple<size_t, size_t, size_t, size_t, size_t, size_t> FirstPassTextures;
    static const STK::Tuple<> ShadowTextures;
    static const STK::Tuple<size_t, size_t, size_t, size_t, size_t> RSMTextures;
};

const STK::Tuple<size_t, size_t, size_t, size_t, size_t, size_t> SplattingMat::FirstPassTextures = STK::Tuple<size_t, size_t, size_t, size_t, size_t, size_t>(1, 2, 3, 4, 5, 6);
const STK::Tuple<> SplattingMat::ShadowTextures;
const STK::Tuple<size_t, size_t, size_t, size_t, size_t> SplattingMat::RSMTextures = STK::Tuple<size_t, size_t, size_t, size_t, size_t>(1, 2, 3, 4, 5);

namespace RenderGeometry
{
    struct TexUnit
    {
        GLuint m_id;
        bool m_premul_alpha;

        TexUnit(GLuint id, bool premul_alpha)
        {
            m_id = id;
            m_premul_alpha = premul_alpha;
        }
    };

    template <typename T>
    std::vector<TexUnit> TexUnits(T curr) // required on older clang versions
    {
        std::vector<TexUnit> v;
        v.push_back(curr);
        return v;
    }

    template <typename T, typename... R>
    std::vector<TexUnit> TexUnits(T curr, R... rest) // required on older clang versions
    {
        std::vector<TexUnit> v;
        v.push_back(curr);
        VTexUnits(v, rest...);
        return v;
    }

    template <typename T, typename... R>
    void VTexUnits(std::vector<TexUnit>& v, T curr, R... rest) // required on older clang versions
    {
        v.push_back(curr);
        VTexUnits(v, rest...);
    }

    template <typename T>
    void VTexUnits(std::vector<TexUnit>& v, T curr)
    {
        v.push_back(curr);
    }
}
using namespace RenderGeometry;


template<typename T, typename...uniforms>
void draw(const T *Shader, const GLMesh *mesh, uniforms... Args)
{
    irr_driver->IncreaseObjectCount();
    GLenum ptype = mesh->PrimitiveType;
    GLenum itype = mesh->IndexType;
    size_t count = mesh->IndexCount;

    Shader->setUniforms(Args...);
    glDrawElementsBaseVertex(ptype, (int)count, itype, (GLvoid *)mesh->vaoOffset, (int)mesh->vaoBaseVertex);
}

template<int...List>
struct custom_unroll_args;

template<>
struct custom_unroll_args<>
{
    template<typename T, typename ...TupleTypes, typename ...Args>
    static void exec(const T *Shader, const STK::Tuple<TupleTypes...> &t, Args... args)
    {
        draw<T>(Shader, STK::tuple_get<0>(t), args...);
    }
};

template<int N, int...List>
struct custom_unroll_args<N, List...>
{
    template<typename T, typename ...TupleTypes, typename ...Args>
    static void exec(const T *Shader, const STK::Tuple<TupleTypes...> &t, Args... args)
    {
        custom_unroll_args<List...>::template exec<T>(Shader, t, STK::tuple_get<N>(t), args...);
    }
};

template<typename T, int N>
struct TexExpander_impl
{
    template<typename...TupleArgs, typename... Args>
    static void ExpandTex(GLMesh &mesh, const STK::Tuple<TupleArgs...> &TexSwizzle, Args... args)
    {
        size_t idx = STK::tuple_get<sizeof...(TupleArgs) - N>(TexSwizzle);
        TexExpander_impl<T, N - 1>::template ExpandTex(mesh, TexSwizzle, args..., getTextureGLuint(mesh.textures[idx]));
    }
};

template<typename T>
struct TexExpander_impl<T, 0>
{
    template<typename...TupleArgs, typename... Args>
    static void ExpandTex(GLMesh &mesh, const STK::Tuple<TupleArgs...> &TexSwizzle, Args... args)
    {
        T::getInstance()->SetTextureUnits(args...);
    }
};

template<typename T>
struct TexExpander
{
    template<typename...TupleArgs, typename... Args>
    static void ExpandTex(GLMesh &mesh, const STK::Tuple<TupleArgs...> &TexSwizzle, Args... args)
    {
        TexExpander_impl<T, sizeof...(TupleArgs)>::ExpandTex(mesh, TexSwizzle, args...);
    }
};


template<typename T, int N>
struct HandleExpander_impl
{
    template<typename...TupleArgs, typename... Args>
    static void Expand(uint64_t *TextureHandles, const STK::Tuple<TupleArgs...> &TexSwizzle, Args... args)
    {
        size_t idx = STK::tuple_get<sizeof...(TupleArgs)-N>(TexSwizzle);
        HandleExpander_impl<T, N - 1>::template Expand(TextureHandles, TexSwizzle, args..., TextureHandles[idx]);
    }
};

template<typename T>
struct HandleExpander_impl<T, 0>
{
    template<typename...TupleArgs, typename... Args>
    static void Expand(uint64_t *TextureHandles, const STK::Tuple<TupleArgs...> &TexSwizzle, Args... args)
    {
        T::getInstance()->SetTextureHandles(args...);
    }
};

template<typename T>
struct HandleExpander
{
    template<typename...TupleArgs, typename... Args>
    static void Expand(uint64_t *TextureHandles, const STK::Tuple<TupleArgs...> &TexSwizzle, Args... args)
    {
        HandleExpander_impl<T, sizeof...(TupleArgs)>::Expand(TextureHandles, TexSwizzle, args...);
    }
};

template<typename T, int ...List>
void renderMeshes1stPass()
{
    auto &meshes = T::List::getInstance()->SolidPass;
    glUseProgram(T::FirstPassShader::getInstance()->Program);
    if (CVS->isARBBaseInstanceUsable())
        glBindVertexArray(VAOManager::getInstance()->getVAO(T::VertexType, false));
    for (unsigned i = 0; i < meshes.size(); i++)
    {
        std::vector<GLuint> Textures;
        std::vector<uint64_t> Handles;
        GLMesh &mesh = *(STK::tuple_get<0>(meshes.at(i)));
        if (!CVS->isARBBaseInstanceUsable())
            glBindVertexArray(mesh.vao);
        if (mesh.VAOType != T::VertexType)
        {
#ifdef DEBUG
            Log::error("Materials", "Wrong vertex Type associed to pass 1 (hint texture : %s)", mesh.textures[0]->getName().getPath().c_str());
#endif
            continue;
        }

        if (CVS->isAZDOEnabled())
            HandleExpander<typename T::FirstPassShader>::template Expand(mesh.TextureHandles, T::FirstPassTextures);
        else
            TexExpander<typename T::FirstPassShader>::template ExpandTex(mesh, T::FirstPassTextures);
        custom_unroll_args<List...>::template exec(T::FirstPassShader::getInstance(), meshes.at(i));
    }
}

template<typename T, typename...Args>
void renderInstancedMeshes1stPass(Args...args)
{
    std::vector<GLMesh *> &meshes = T::InstancedList::getInstance()->SolidPass;
    glUseProgram(T::InstancedFirstPassShader::getInstance()->Program);
    glBindVertexArray(VAOManager::getInstance()->getInstanceVAO(T::VertexType, false, T::Instance));
    for (unsigned i = 0; i < meshes.size(); i++)
    {
        std::vector<GLuint> Textures;
        GLMesh *mesh = meshes[i];
#ifdef DEBUG
        if (mesh->VAOType != T::VertexType)
        {
            Log::error("RenderGeometry", "Wrong instanced vertex format (hint : %s)", 
                mesh->textures[0]->getName().getPath().c_str());
            continue;
        }
#endif
        TexExpander<typename T::InstancedFirstPassShader>::template ExpandTex(*mesh, T::FirstPassTextures);

        T::InstancedFirstPassShader::getInstance()->setUniforms(args...);
        glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, (const void*)((SolidPassCmd::getInstance()->Offset[T::MaterialType] + i) * sizeof(DrawElementsIndirectCommand)));
    }
}

template<typename T, typename...Args>
void multidraw1stPass(Args...args)
{
    glUseProgram(T::InstancedFirstPassShader::getInstance()->Program);
    glBindVertexArray(VAOManager::getInstance()->getInstanceVAO(T::VertexType, false, T::Instance));
    if (SolidPassCmd::getInstance()->Size[T::MaterialType])
    {
        T::InstancedFirstPassShader::getInstance()->setUniforms(args...);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT,
            (const void*)(SolidPassCmd::getInstance()->Offset[T::MaterialType] * sizeof(DrawElementsIndirectCommand)),
            (int)SolidPassCmd::getInstance()->Size[T::MaterialType],
            sizeof(DrawElementsIndirectCommand));
    }
}

static core::vector3df windDir;

void IrrDriver::renderGBuffer()
{
    windDir = getWindDir();

    if (CVS->supportsIndirectInstancingRendering())
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, SolidPassCmd::getInstance()->drawindirectcmd);

    {
        ScopedGPUTimer Timer(getGPUTimer(Q_SOLID_PASS1));
        irr_driver->setPhase(GBUFFER_PASS);

        for (unsigned i = 0; i < ImmediateDrawList::getInstance()->size(); i++)
            ImmediateDrawList::getInstance()->at(i)->render();

        renderMeshes1stPass<DefaultMaterial, 3, 2, 1>();
        renderMeshes1stPass<SplattingMat, 2, 1>();
        renderMeshes1stPass<UnlitMat, 3, 2, 1>();
        renderMeshes1stPass<AlphaRef, 3, 2, 1>();

        renderMeshes1stPass<DetailMat, 2, 1>();

        // Skinned meshes
        {
            if (CVS->isARBBaseInstanceUsable())
                glBindVertexArray(VAOManager::getInstance()->getVAO(video::EVT_STANDARD, true));
            for (STKAnimatedMesh *animatedmesh : *(ListSkinned::getInstance()))
            {
                core::matrix4 ModelMatrix = animatedmesh->getAbsoluteTransformation(), InvModelMatrix;
                ModelMatrix.getInverse(InvModelMatrix);

                glUseProgram(MeshShader::SkinnedObjectShader::getInstance()->Program);
                for (GLMesh *mesh : animatedmesh->MeshSolidMaterial[Material::SHADERTYPE_SOLID])
                {
                    if (!CVS->isARBBaseInstanceUsable())
                        glBindVertexArray(mesh->vao);
                    if (!CVS->isAZDOEnabled())
                        MeshShader::SkinnedObjectShader::getInstance()->SetTextureUnits(getTextureGLuint(mesh->textures[0]), getTextureGLuint(mesh->textures[1]));
                    else
                        MeshShader::SkinnedObjectShader::getInstance()->SetTextureHandles(mesh->TextureHandles[0], mesh->TextureHandles[1]);
                    MeshShader::SkinnedObjectShader::getInstance()->setUniforms(ModelMatrix, InvModelMatrix, *(animatedmesh->JointMatrixes));
                    glDrawElementsBaseVertex(mesh->PrimitiveType, (int)mesh->IndexCount, mesh->IndexType, (GLvoid *)mesh->vaoOffset, (int)mesh->vaoBaseVertex);
                }

                glUseProgram(MeshShader::SkinnedRefObjectShader::getInstance()->Program);
                for (GLMesh *mesh : animatedmesh->MeshSolidMaterial[Material::SHADERTYPE_ALPHA_TEST])
                {
                    if (!CVS->isARBBaseInstanceUsable())
                        glBindVertexArray(mesh->vao);
                    if (!CVS->isAZDOEnabled())
                        MeshShader::SkinnedRefObjectShader::getInstance()->SetTextureUnits(getTextureGLuint(mesh->textures[0]), getTextureGLuint(mesh->textures[1]));
                    else
                        MeshShader::SkinnedRefObjectShader::getInstance()->SetTextureHandles(mesh->TextureHandles[0], mesh->TextureHandles[1]);
                    MeshShader::SkinnedRefObjectShader::getInstance()->setUniforms(ModelMatrix, InvModelMatrix, *(animatedmesh->JointMatrixes));
                    glDrawElementsBaseVertex(mesh->PrimitiveType, (int)mesh->IndexCount, mesh->IndexType, (GLvoid *)mesh->vaoOffset, (int)mesh->vaoBaseVertex);
                }
            }
        }

        if (CVS->isAZDOEnabled())
        {
            multidraw1stPass<DefaultMaterial>();
            multidraw1stPass<AlphaRef>();
            multidraw1stPass<UnlitMat>();
            multidraw1stPass<NormalMat>();
            multidraw1stPass<DetailMat>();
        }
        else if (CVS->supportsIndirectInstancingRendering())
        {
            renderInstancedMeshes1stPass<DefaultMaterial>();
            renderInstancedMeshes1stPass<AlphaRef>();
            renderInstancedMeshes1stPass<UnlitMat>();
            renderInstancedMeshes1stPass<DetailMat>();
            renderInstancedMeshes1stPass<NormalMat>();
        }

        // Subsurface
        glEnable(GL_STENCIL_TEST);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, 2, 0xFF);
        renderMeshes1stPass<NormalMat, 2, 1>();
        if (CVS->isAZDOEnabled())
            multidraw1stPass<NormalMat>();
        else if (CVS->supportsIndirectInstancingRendering())
            renderInstancedMeshes1stPass<NormalMat>();

        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        renderMeshes1stPass<GrassMat, 3, 2, 1>();
        if (CVS->isAZDOEnabled())
            multidraw1stPass<GrassMat>(windDir);
        else if (CVS->supportsIndirectInstancingRendering())
            renderInstancedMeshes1stPass<GrassMat>(windDir);
        glDisable(GL_STENCIL_TEST);
    }
}

void IrrDriver::renderPostLightFixups()
{
    // Render unlit material
   {
       glEnable(GL_DEPTH_TEST);
       auto &meshes = UnlitMat::List::getInstance()->SolidPass;
       glUseProgram(MeshShader::ObjectUnlitShader::getInstance()->Program);
       if (CVS->isARBBaseInstanceUsable())
           glBindVertexArray(VAOManager::getInstance()->getVAO(UnlitMat::VertexType, false));
       for (unsigned i = 0; i < meshes.size(); i++)
       {
           GLMesh &mesh = *(STK::tuple_get<0>(meshes.at(i)));
           if (!CVS->isARBBaseInstanceUsable())
               glBindVertexArray(mesh.vao);

           if (mesh.VAOType != UnlitMat::VertexType)
           {
#ifdef DEBUG
               Log::error("Materials", "Wrong vertex Type associed to pass 2 (hint texture : %s)", mesh.textures[0]->getName().getPath().c_str());
#endif
               continue;
           }

           if (UserConfigParams::m_azdo)
               HandleExpander<MeshShader::ObjectUnlitShader>::Expand(mesh.TextureHandles, STK::make_tuple(0));
           else
               TexExpander<MeshShader::ObjectUnlitShader>::ExpandTex(mesh, STK::make_tuple(0));
           MeshShader::ObjectUnlitShader::getInstance()->setUniforms(STK::tuple_get<1>(meshes.at(i)), STK::tuple_get<3>(meshes.at(i)));
           glDrawElementsBaseVertex(mesh.PrimitiveType, (int)mesh.IndexCount, mesh.IndexType, (GLvoid *)mesh.vaoOffset, (int)mesh.vaoBaseVertex);
       }

       if (UserConfigParams::m_azdo)
       {
           glUseProgram(MeshShader::InstancedObjectUnlitShader::getInstance()->Program);
           glBindVertexArray(VAOManager::getInstance()->getInstanceVAO(UnlitMat::VertexType, false, UnlitMat::Instance));
           uint64_t nulltex[10] = {};
           if (SolidPassCmd::getInstance()->Size[UnlitMat::MaterialType])
           {
               HandleExpander<MeshShader::InstancedObjectUnlitShader>::Expand(nulltex, STK::make_tuple(0));
               MeshShader::InstancedObjectUnlitShader::getInstance()->setUniforms();
               glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT,
                   (const void*)(SolidPassCmd::getInstance()->Offset[UnlitMat::MaterialType] * sizeof(DrawElementsIndirectCommand)),
                   (int)SolidPassCmd::getInstance()->Size[UnlitMat::MaterialType],
                   (int)sizeof(DrawElementsIndirectCommand));
           }
       }
       else if (CVS->supportsIndirectInstancingRendering())
       {
           std::vector<GLMesh *> &meshes = UnlitMat::InstancedList::getInstance()->SolidPass;
           glUseProgram(MeshShader::InstancedObjectUnlitShader::getInstance()->Program);
           glBindVertexArray(VAOManager::getInstance()->getInstanceVAO(UnlitMat::VertexType, false, UnlitMat::Instance));
           for (unsigned i = 0; i < meshes.size(); i++)
           {
               GLMesh *mesh = meshes[i];
               TexExpander<MeshShader::InstancedObjectUnlitShader>::ExpandTex(*mesh, STK::make_tuple(0));
               MeshShader::InstancedObjectUnlitShader::getInstance()->setUniforms();
               glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, (const void*)((SolidPassCmd::getInstance()->Offset[UnlitMat::MaterialType] + i) * sizeof(DrawElementsIndirectCommand)));
           }
       }
   }
}

template<typename T>
static void renderInstancedMeshNormals()
{
    std::vector<GLMesh *> &meshes = T::InstancedList::getInstance()->SolidPass;
    glUseProgram(MeshShader::NormalVisualizer::getInstance()->Program);
    glBindVertexArray(VAOManager::getInstance()->getInstanceVAO(T::VertexType, false, T::Instance));
    for (unsigned i = 0; i < meshes.size(); i++)
    {
        MeshShader::NormalVisualizer::getInstance()->setUniforms(video::SColor(255, 0, 255, 0));
        glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, (const void*)((SolidPassCmd::getInstance()->Offset[T::MaterialType] + i) * sizeof(DrawElementsIndirectCommand)));
    }
}

template<typename T>
static void renderMultiMeshNormals()
{
    glUseProgram(MeshShader::NormalVisualizer::getInstance()->Program);
    glBindVertexArray(VAOManager::getInstance()->getInstanceVAO(T::VertexType, false, T::Instance));
    if (SolidPassCmd::getInstance()->Size[T::MaterialType])
    {
        MeshShader::NormalVisualizer::getInstance()->setUniforms(video::SColor(255, 0, 255, 0));
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT,
            (const void*)(SolidPassCmd::getInstance()->Offset[T::MaterialType] * sizeof(DrawElementsIndirectCommand)),
            (int)SolidPassCmd::getInstance()->Size[T::MaterialType],
            (int)sizeof(DrawElementsIndirectCommand));
    }
}

void IrrDriver::renderNormalsVisualisation()
{
    if (CVS->isAZDOEnabled()) {
        renderMultiMeshNormals<DefaultMaterial>();
        renderMultiMeshNormals<AlphaRef>();
        renderMultiMeshNormals<UnlitMat>();
        renderMultiMeshNormals<DetailMat>();
        renderMultiMeshNormals<NormalMat>();
    }
    else if (CVS->supportsIndirectInstancingRendering())
    {
        renderInstancedMeshNormals<DefaultMaterial>();
        renderInstancedMeshNormals<AlphaRef>();
        renderInstancedMeshNormals<UnlitMat>();
        renderInstancedMeshNormals<DetailMat>();
        renderInstancedMeshNormals<NormalMat>();
    }
}

template<typename Shader, enum video::E_VERTEX_TYPE VertexType, int...List, typename... TupleType>
void renderTransparenPass(const std::vector<TexUnit> &TexUnits, std::vector<STK::Tuple<TupleType...> > *meshes)
{
    glUseProgram(Shader::getInstance()->Program);
    if (CVS->isARBBaseInstanceUsable())
        glBindVertexArray(VAOManager::getInstance()->getVAO(VertexType, false));
    for (unsigned i = 0; i < meshes->size(); i++)
    {
        GLMesh &mesh = *(STK::tuple_get<0>(meshes->at(i)));
        if (!CVS->isARBBaseInstanceUsable())
            glBindVertexArray(mesh.vao);
        if (mesh.VAOType != VertexType)
        {
#ifdef DEBUG
            Log::error("Materials", "Wrong vertex Type associed to pass 2 (hint texture : %s)", mesh.textures[0]->getName().getPath().c_str());
#endif
            continue;
        }

        if (CVS->isAZDOEnabled())
            Shader::getInstance()->SetTextureHandles(mesh.TextureHandles[0], irr_driver->SkyboxSpecularProbeHandle, irr_driver->DFG_LUT_Handle);
        else
            Shader::getInstance()->SetTextureUnits(getTextureGLuint(mesh.textures[0]), irr_driver->SkyboxSpecularProbe, irr_driver->DFG_LUT);
        custom_unroll_args<List...>::template exec(Shader::getInstance(), meshes->at(i));
    }
}

static video::ITexture *displaceTex = 0;

void IrrDriver::renderTransparent()
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glDisable(GL_CULL_FACE);

    irr_driver->setPhase(TRANSPARENT_PASS);

    for (unsigned i = 0; i < ImmediateDrawList::getInstance()->size(); i++)
        ImmediateDrawList::getInstance()->at(i)->render();

    if (CVS->isARBBaseInstanceUsable())
        glBindVertexArray(VAOManager::getInstance()->getVAO(video::EVT_STANDARD, false));

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    renderTransparenPass<MeshShader::TransparentFogShader, video::EVT_STANDARD, 3, 2, 1>(TexUnits(
        TexUnit(0, true)), ListBlendTransparentFog::getInstance());
    glBlendFunc(GL_ONE, GL_ONE);
    renderTransparenPass<MeshShader::TransparentFogShader, video::EVT_STANDARD, 3, 2, 1>(TexUnits(
        TexUnit(0, true)), ListAdditiveTransparentFog::getInstance());

    for (unsigned i = 0; i < BillBoardList::getInstance()->size(); i++)
        BillBoardList::getInstance()->at(i)->render();

    if (!UserConfigParams::m_dynamic_lights)
        return;

    // Render displacement nodes
    irr_driver->getFBO(FBO_TMP1_WITH_DS).Bind();
    glClear(GL_COLOR_BUFFER_BIT);
    irr_driver->getFBO(FBO_DISPLACE).Bind();
    glClear(GL_COLOR_BUFFER_BIT);

    DisplaceProvider * const cb = (DisplaceProvider *)irr_driver->getCallback(ES_DISPLACE);
    cb->update();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glClear(GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    if (CVS->isARBBaseInstanceUsable())
        glBindVertexArray(VAOManager::getInstance()->getVAO(video::EVT_2TCOORDS, false));
    // Generate displace mask
    // Use RTT_TMP4 as displace mask
    irr_driver->getFBO(FBO_TMP1_WITH_DS).Bind();
    for (unsigned i = 0; i < ListDisplacement::getInstance()->size(); i++)
    {
        const GLMesh &mesh = *(STK::tuple_get<0>(ListDisplacement::getInstance()->at(i)));
        if (!CVS->isARBBaseInstanceUsable())
            glBindVertexArray(mesh.vao);
        const core::matrix4 &AbsoluteTransformation = STK::tuple_get<1>(ListDisplacement::getInstance()->at(i));
        if (mesh.VAOType != video::EVT_2TCOORDS)
        {
#ifdef DEBUG
            Log::error("Materials", "Displacement has wrong vertex type");
#endif
            continue;
        }

        GLenum ptype = mesh.PrimitiveType;
        GLenum itype = mesh.IndexType;
        size_t count = mesh.IndexCount;

        glUseProgram(MeshShader::DisplaceMaskShader::getInstance()->Program);
        MeshShader::DisplaceMaskShader::getInstance()->setUniforms(AbsoluteTransformation);
        glDrawElementsBaseVertex(ptype, (int)count, itype,
                                 (GLvoid *)mesh.vaoOffset, (int)mesh.vaoBaseVertex);
    }

    irr_driver->getFBO(FBO_DISPLACE).Bind();
    if (!displaceTex)
        displaceTex = irr_driver->getTexture(FileManager::TEXTURE, "displace.png");
    for (unsigned i = 0; i < ListDisplacement::getInstance()->size(); i++)
    {
        const GLMesh &mesh = *(STK::tuple_get<0>(ListDisplacement::getInstance()->at(i)));
        if (!CVS->isARBBaseInstanceUsable())
            glBindVertexArray(mesh.vao);
        const core::matrix4 &AbsoluteTransformation = STK::tuple_get<1>(ListDisplacement::getInstance()->at(i));
        if (mesh.VAOType != video::EVT_2TCOORDS)
            continue;

        GLenum ptype = mesh.PrimitiveType;
        GLenum itype = mesh.IndexType;
        size_t count = mesh.IndexCount;
        // Render the effect
        MeshShader::DisplaceShader::getInstance()->SetTextureUnits(
            getTextureGLuint(displaceTex),
            irr_driver->getRenderTargetTexture(RTT_COLOR),
            irr_driver->getRenderTargetTexture(RTT_TMP1),
            getTextureGLuint(mesh.textures[0]));
        glUseProgram(MeshShader::DisplaceShader::getInstance()->Program);
        MeshShader::DisplaceShader::getInstance()->setUniforms(AbsoluteTransformation,
            core::vector2df(cb->getDirX(), cb->getDirY()),
            core::vector2df(cb->getDir2X(), cb->getDir2Y()));

        glDrawElementsBaseVertex(ptype, (int)count, itype, (GLvoid *)mesh.vaoOffset, (int)mesh.vaoBaseVertex);
    }

    irr_driver->getFBO(FBO_COLORS).Bind();
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    m_post_processing->renderPassThrough(m_rtts->getRenderTarget(RTT_DISPLACE));
    glDisable(GL_STENCIL_TEST);

}

template<typename T, typename...uniforms>
void drawShadow(const T *Shader, unsigned cascade, const GLMesh *mesh, uniforms... Args)
{
    irr_driver->IncreaseObjectCount();
    GLenum ptype = mesh->PrimitiveType;
    GLenum itype = mesh->IndexType;
    size_t count = mesh->IndexCount;

    Shader->setUniforms(cascade, Args...);
    glDrawElementsBaseVertex(ptype, (int)count, itype, (GLvoid *)mesh->vaoOffset, (int)mesh->vaoBaseVertex);
}

template<int...List>
struct shadow_custom_unroll_args;

template<>
struct shadow_custom_unroll_args<>
{
    template<typename T, typename ...TupleTypes, typename ...Args>
    static void exec(const T *Shader, unsigned cascade, const STK::Tuple<TupleTypes...> &t, Args... args)
    {
        drawShadow<T>(Shader, cascade, STK::tuple_get<0>(t), args...);
    }
};

template<int N, int...List>
struct shadow_custom_unroll_args<N, List...>
{
    template<typename T, typename ...TupleTypes, typename ...Args>
    static void exec(const T *Shader, unsigned cascade, const STK::Tuple<TupleTypes...> &t, Args... args)
    {
        shadow_custom_unroll_args<List...>::template exec<T>(Shader, cascade, t, STK::tuple_get<N>(t), args...);
    }
};

template<typename T, int...List>
void renderShadow(unsigned cascade)
{
    auto &t = T::List::getInstance()->Shadows[cascade];
    glUseProgram(T::ShadowPassShader::getInstance()->Program);
    if (CVS->isARBBaseInstanceUsable())
        glBindVertexArray(VAOManager::getInstance()->getVAO(T::VertexType, false));
    for (unsigned i = 0; i < t.size(); i++)
    {
        GLMesh *mesh = STK::tuple_get<0>(t.at(i));
        if (!CVS->isARBBaseInstanceUsable())
            glBindVertexArray(mesh->vao);
        if (CVS->isAZDOEnabled())
            HandleExpander<typename T::ShadowPassShader>::template Expand(mesh->TextureHandles, T::ShadowTextures);
        else
            TexExpander<typename T::ShadowPassShader>::template ExpandTex(*mesh, T::ShadowTextures);
        shadow_custom_unroll_args<List...>::template exec<typename T::ShadowPassShader>(T::ShadowPassShader::getInstance(), cascade, t.at(i));
    }
}

template<typename T, typename...Args>
void renderInstancedShadow(unsigned cascade, Args ...args)
{
    glUseProgram(T::InstancedShadowPassShader::getInstance()->Program);
    glBindVertexArray(VAOManager::getInstance()->getInstanceVAO(T::VertexType, false, InstanceTypeShadow));
    std::vector<GLMesh *> &t = T::InstancedList::getInstance()->Shadows[cascade];
    for (unsigned i = 0; i < t.size(); i++)
    {
        GLMesh *mesh = t[i];

        TexExpander<typename T::InstancedShadowPassShader>::template ExpandTex(*mesh, T::ShadowTextures);
        T::InstancedShadowPassShader::getInstance()->setUniforms(cascade, args...);
        size_t tmp = ShadowPassCmd::getInstance()->Offset[cascade][T::MaterialType] + i;
        glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, (const void*)((tmp) * sizeof(DrawElementsIndirectCommand)));
    }

}

template<typename T, typename...Args>
static void multidrawShadow(unsigned i, Args ...args)
{
    glUseProgram(T::InstancedShadowPassShader::getInstance()->Program);
    glBindVertexArray(VAOManager::getInstance()->getInstanceVAO(T::VertexType, false, InstanceTypeShadow));
    if (ShadowPassCmd::getInstance()->Size[i][T::MaterialType])
    {
        T::InstancedShadowPassShader::getInstance()->setUniforms(i, args...);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, 
            (const void*)(ShadowPassCmd::getInstance()->Offset[i][T::MaterialType] * sizeof(DrawElementsIndirectCommand)),
            (int)ShadowPassCmd::getInstance()->Size[i][T::MaterialType], sizeof(DrawElementsIndirectCommand));
    }
}

void IrrDriver::renderShadows()
{
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    m_rtts->getShadowFBO().Bind();
    if (!CVS->isESMEnabled())
    {
        glDrawBuffer(GL_NONE);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.5, 50.);
    }
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    glClearColor(1., 1., 1., 1.);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glClearColor(0., 0., 0., 0.);

    for (unsigned cascade = 0; cascade < 4; cascade++)
    {
        ScopedGPUTimer Timer(getGPUTimer(Q_SHADOWS_CASCADE0 + cascade));

        renderShadow<DefaultMaterial, 1>(cascade);
        renderShadow<DetailMat, 1>(cascade);
        renderShadow<SplattingMat, 1>(cascade);
        renderShadow<NormalMat, 1>(cascade);
        renderShadow<AlphaRef, 1>(cascade);
        renderShadow<UnlitMat, 1>(cascade);
        renderShadow<GrassMat, 3, 1>(cascade);

        if (CVS->supportsIndirectInstancingRendering())
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ShadowPassCmd::getInstance()->drawindirectcmd);

        if (CVS->isAZDOEnabled())
        {
            multidrawShadow<DefaultMaterial>(cascade);
            multidrawShadow<DetailMat>(cascade);
            multidrawShadow<NormalMat>(cascade);
            multidrawShadow<AlphaRef>(cascade);
            multidrawShadow<UnlitMat>(cascade);
            multidrawShadow<GrassMat>(cascade, windDir);
        }
        else if (CVS->supportsIndirectInstancingRendering())
        {
            renderInstancedShadow<DefaultMaterial>(cascade);
            renderInstancedShadow<DetailMat>(cascade);
            renderInstancedShadow<AlphaRef>(cascade);
            renderInstancedShadow<UnlitMat>(cascade);
            renderInstancedShadow<GrassMat>(cascade, windDir);
            renderInstancedShadow<NormalMat>(cascade);
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    return;

    if (CVS->isESMEnabled())
    {
        ScopedGPUTimer Timer(getGPUTimer(Q_SHADOW_POSTPROCESS));

        if (CVS->isARBTextureViewUsable())
        {
            for (unsigned i = 0; i < 2; i++)
            {
                m_post_processing->renderGaussian6BlurLayer(m_rtts->getShadowFBO(), i,
                    2.f * m_shadow_scales[0].first / m_shadow_scales[i].first,
                    2.f * m_shadow_scales[0].second / m_shadow_scales[i].second);
            }
        }
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_rtts->getShadowFBO().getRTT()[0]);
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    }
}



template<int...List>
struct rsm_custom_unroll_args;

template<>
struct rsm_custom_unroll_args<>
{
    template<typename T, typename ...TupleTypes, typename ...Args>
    static void exec(const core::matrix4 &rsm_matrix, const STK::Tuple<TupleTypes...> &t, Args... args)
    {
        draw<T>(T::getInstance(), STK::tuple_get<0>(t), rsm_matrix, args...);
    }
};

template<int N, int...List>
struct rsm_custom_unroll_args<N, List...>
{
    template<typename T, typename ...TupleTypes, typename ...Args>
    static void exec(const core::matrix4 &rsm_matrix, const STK::Tuple<TupleTypes...> &t, Args... args)
    {
        rsm_custom_unroll_args<List...>::template exec<T>(rsm_matrix, t, STK::tuple_get<N>(t), args...);
    }
};

template<typename T, int... Selector>
void drawRSM(const core::matrix4 & rsm_matrix)
{
    glUseProgram(T::RSMShader::getInstance()->Program);
    if (CVS->isARBBaseInstanceUsable())
        glBindVertexArray(VAOManager::getInstance()->getVAO(T::VertexType, false));
    auto t = T::List::getInstance()->RSM;
    for (unsigned i = 0; i < t.size(); i++)
    {
        std::vector<GLuint> Textures;
        GLMesh *mesh = STK::tuple_get<0>(t.at(i));
        if (!CVS->isARBBaseInstanceUsable())
            glBindVertexArray(mesh->vao);
        if (CVS->isAZDOEnabled())
            HandleExpander<typename T::RSMShader>::template Expand(mesh->TextureHandles, T::RSMTextures);
        else
            TexExpander<typename T::RSMShader>::template ExpandTex(*mesh, T::RSMTextures);
        rsm_custom_unroll_args<Selector...>::template exec<typename T::RSMShader>(rsm_matrix, t.at(i));
    }
}

template<typename T, typename...Args>
void renderRSMShadow(Args ...args)
{
    glUseProgram(T::InstancedRSMShader::getInstance()->Program);
    glBindVertexArray(VAOManager::getInstance()->getInstanceVAO(T::VertexType, false, InstanceTypeRSM));
    auto t = T::InstancedList::getInstance()->RSM;
    for (unsigned i = 0; i < t.size(); i++)
    {
        std::vector<GLuint> Textures;
        GLMesh *mesh = t[i];

        TexExpander<typename T::InstancedRSMShader>::template ExpandTex(*mesh, T::RSMTextures);
        T::InstancedRSMShader::getInstance()->setUniforms(args...);
        glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, (const void*)((RSMPassCmd::getInstance()->Offset[T::MaterialType] + i)* sizeof(DrawElementsIndirectCommand)));
    }
}

template<typename T, typename... Args>
void multidrawRSM(Args...args)
{
    glUseProgram(T::InstancedRSMShader::getInstance()->Program);
    glBindVertexArray(VAOManager::getInstance()->getInstanceVAO(T::VertexType, false, InstanceTypeRSM));
    if (RSMPassCmd::getInstance()->Size[T::MaterialType])
    {
        T::InstancedRSMShader::getInstance()->setUniforms(args...);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT,
            (const void*)(RSMPassCmd::getInstance()->Offset[T::MaterialType] * sizeof(DrawElementsIndirectCommand)),
            (int)RSMPassCmd::getInstance()->Size[T::MaterialType], sizeof(DrawElementsIndirectCommand));
    }
}

void IrrDriver::renderRSM()
{
    if (m_rsm_map_available)
        return;
    ScopedGPUTimer Timer(getGPUTimer(Q_RSM));
    m_rtts->getRSM().Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawRSM<DefaultMaterial, 3, 1>(rsm_matrix);
    drawRSM<AlphaRef, 3, 1>(rsm_matrix);
    drawRSM<NormalMat, 3, 1>(rsm_matrix);
    drawRSM<UnlitMat, 3, 1>(rsm_matrix);
    drawRSM<DetailMat, 3, 1>(rsm_matrix);
    drawRSM<SplattingMat, 1>(rsm_matrix);

    if (CVS->supportsIndirectInstancingRendering())
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, RSMPassCmd::getInstance()->drawindirectcmd);

    if (CVS->isAZDOEnabled())
    {
        multidrawRSM<DefaultMaterial>(rsm_matrix);
        multidrawRSM<NormalMat>(rsm_matrix);
        multidrawRSM<AlphaRef>(rsm_matrix);
        multidrawRSM<UnlitMat>(rsm_matrix);
        multidrawRSM<DetailMat>(rsm_matrix);
    }
    else if (CVS->supportsIndirectInstancingRendering())
    {
        renderRSMShadow<DefaultMaterial>(rsm_matrix);
        renderRSMShadow<AlphaRef>(rsm_matrix);
        renderRSMShadow<UnlitMat>(rsm_matrix);
        renderRSMShadow<NormalMat>(rsm_matrix);
        renderRSMShadow<DetailMat>(rsm_matrix);
    }
    m_rsm_map_available = true;
}
