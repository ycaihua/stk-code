#include "central_settings.hpp"
#include "graphics/glwrap.hpp"
#include "graphics/stkanimatedmesh.hpp"
#include <ISceneManager.h>
#include <IMaterialRenderer.h>
#include <ISkinnedMesh.h>
#include "graphics/irr_driver.hpp"
#include "graphics/material_manager.hpp"
#include "config/user_config.hpp"
#include "modes/world.hpp"
#include "tracks/track.hpp"
#include "graphics/camera.hpp"
#include "utils/profiler.hpp"
#include "utils/cpp2011.hpp"
#include "../lib/irrlicht/source/Irrlicht/CSkinnedMesh.h"

using namespace irr;

STKAnimatedMesh::STKAnimatedMesh(irr::scene::IAnimatedMesh* mesh, irr::scene::ISceneNode* parent,
irr::scene::ISceneManager* mgr, s32 id, const std::string& debug_name,
const core::vector3df& position,
const core::vector3df& rotation,
const core::vector3df& scale) :
    CAnimatedMeshSceneNode(mesh, parent, mgr, id, position, rotation, scale)
{
    isGLInitialized = false;
    isMaterialInitialized = false;
#ifdef DEBUG
    m_debug_name = debug_name;
#endif
}

STKAnimatedMesh::~STKAnimatedMesh()
{
    cleanGLMeshes();
}

void STKAnimatedMesh::cleanGLMeshes()
{
    for (u32 i = 0; i < GLmeshes.size(); ++i)
    {
        GLMesh mesh = GLmeshes[i];
        if (!mesh.vertex_buffer)
            continue;
        if (mesh.vao)
            glDeleteVertexArrays(1, &(mesh.vao));
        if (mesh.vertex_buffer)
            glDeleteBuffers(1, &(mesh.vertex_buffer));
        if (mesh.index_buffer)
            glDeleteBuffers(1, &(mesh.index_buffer));
    }
    GLmeshes.clear();
    for (unsigned i = 0; i < Material::SHADERTYPE_COUNT; i++)
        MeshSolidMaterial[i].clearWithoutDeleting();
    for (unsigned i = 0; i < TM_COUNT; i++)
        TransparentMesh[i].clearWithoutDeleting();
}

void STKAnimatedMesh::setMesh(scene::IAnimatedMesh* mesh)
{
    isGLInitialized = false;
    isMaterialInitialized = false;
    cleanGLMeshes();
    CAnimatedMeshSceneNode::setMesh(mesh);
}

void STKAnimatedMesh::updateNoGL()
{
    scene::IMesh* m = getMeshForCurrentFrame();

    if (m)
        Box = m->getBoundingBox();
    else
    {
        Log::error("animated mesh", "Animated Mesh returned no mesh to render.");
        return;
    }

    if (!isMaterialInitialized)
    {
        video::IVideoDriver* driver = SceneManager->getVideoDriver();
        for (u32 i = 0; i < m->getMeshBufferCount(); ++i)
        {
            scene::IMeshBuffer* mb = Mesh->getMeshBuffer(i);
            GLmeshes.push_back(allocateMeshBuffer(mb, m_debug_name));
        }

        for (u32 i = 0; i < m->getMeshBufferCount(); ++i)
        {
            scene::IMeshBuffer* mb = Mesh->getMeshBuffer(i);
            if (!mb)
                continue;
            video::E_MATERIAL_TYPE type = mb->getMaterial().MaterialType;
            f32 MaterialTypeParam = mb->getMaterial().MaterialTypeParam;
            video::IMaterialRenderer* rnd = driver->getMaterialRenderer(type);
            if (!isObject(type))
            {
#ifdef DEBUG
                Log::warn("material", "Unhandled (animated) material type : %d", type);
#endif
                continue;
            }
            GLMesh &mesh = GLmeshes[i];
            Material* material = material_manager->getMaterialFor(mb->getMaterial().getTexture(0), mb);

            if (rnd->isTransparent())
            {
                TransparentMaterial TranspMat = MaterialTypeToTransparentMaterial(type, MaterialTypeParam, material);
                TransparentMesh[TranspMat].push_back(&mesh);
            }
            else
            {
                Material::ShaderType MatType = material->getShaderType();// MaterialTypeToMeshMaterial(type, mb->getVertexType(), material);
                MeshSolidMaterial[MatType].push_back(&mesh);
            }
        }
        isMaterialInitialized = true;
    }

    for (u32 i = 0; i < m->getMeshBufferCount(); ++i)
    {
        scene::IMeshBuffer* mb = Mesh->getMeshBuffer(i);
        if (!mb)
            continue;
        if (mb)
            GLmeshes[i].TextureMatrix = getMaterial(i).getTextureMatrix(0);
    }

    scene::CSkinnedMesh *SkinnedMesh = dynamic_cast<scene::CSkinnedMesh *>(Mesh);
    JointMatrixes = &(SkinnedMesh->JointMatrixes);
}

void STKAnimatedMesh::updateGL()
{

    scene::IMesh* m = getMeshForCurrentFrame();

    if (!isGLInitialized)
    {
        // Retrieve idle pose
        scene::CSkinnedMesh *SkinnedMesh = dynamic_cast<scene::CSkinnedMesh *>(Mesh);
        SkinnedMesh->SkinnedLastFrame = false;
        SkinnedMesh->areWeightGenerated = false;
        SkinnedMesh->skinMesh(0.);

        for (u32 i = 0; i < Mesh->getMeshBufferCount(); ++i)
        {
            scene::IMeshBuffer* mb = Mesh->getMeshBuffer(i);
            if (!mb)
                continue;
            video::IVideoDriver* driver = SceneManager->getVideoDriver();
            video::E_MATERIAL_TYPE type = mb->getMaterial().MaterialType;
            video::IMaterialRenderer* rnd = driver->getMaterialRenderer(type);
            GLMesh &mesh = GLmeshes[i];

            if (!rnd->isTransparent())
            {
                Material* material = material_manager->getMaterialFor(mb->getMaterial().getTexture(0), mb);
                Material* material2 = NULL;
                if (mb->getMaterial().getTexture(1) != NULL)
                    material2 = material_manager->getMaterialFor(mb->getMaterial().getTexture(1), mb);

                Material::ShaderType MatType = MaterialTypeToMeshMaterial(type, mb->getVertexType(), material, material2);
                InitTextures(mesh, MatType);
            }
            else
                InitTexturesTransparent(mesh);

            if (CVS->isARBBaseInstanceUsable())
            {
                std::vector<scene::JointInfluence> reworkedWeightInfluence;
                // Some empty mesh are sometimes submitted, no idea why
                if (SkinnedMesh->WeightInfluence.size() > i)
                {
                    for (unsigned idx = 0; idx < SkinnedMesh->WeightInfluence[i].size(); idx++)
                    {
                        std::vector<scene::JointInfluence> ReportedWeight = SkinnedMesh->WeightInfluence[i][idx];
                        std::sort(ReportedWeight.begin(), ReportedWeight.end(), [](const scene::JointInfluence &a, const scene::JointInfluence &b) {return a.weight > b .weight; });
                        float remaining_weight = 1.;
                        for (unsigned k = 0; k < 4; k++)
                        {
                            scene::JointInfluence influence;
                            if (ReportedWeight.size() > k)
                               influence = ReportedWeight[k];
                            else
                            {
                                influence.JointIdx = -1;
                                influence.weight = remaining_weight;
                            }
                            remaining_weight -= influence.weight;
                            reworkedWeightInfluence.push_back(influence);
                        }
                    }

                    std::pair<unsigned, unsigned> p = VAOManager::getInstance()->getBase(mb, reworkedWeightInfluence.data());
                    mesh.vaoBaseVertex = p.first;
                    mesh.vaoOffset = p.second;
                }
            }
            else
            {
                fillLocalBuffer(mesh, mb);
                mesh.vao = createVAO(mesh.vertex_buffer, mesh.index_buffer, mb->getVertexType());
                glBindVertexArray(0);
            }
        }
        isGLInitialized = true;
    }
}

void STKAnimatedMesh::render()
{
    ++PassCount;

    updateNoGL();
    updateGL();
}
