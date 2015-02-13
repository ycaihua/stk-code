#include "graphics/glwrap.hpp"
#include "graphics/stkscenemanager.hpp"
#include "graphics/stkmesh.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/central_settings.hpp"
#include "stkanimatedmesh.hpp"
#include "stkmeshscenenode.hpp"
#include "utils/ptr_vector.hpp"
#include <SceneNodes/ICameraSceneNode.h>
#include <Core/SViewFrustum.h>
#include "callbacks.hpp"
#include "utils/cpp2011.hpp"
#include "modes/world.hpp"
#include "tracks/track.hpp"
#include "lod_node.hpp"
#include "utils/profiler.hpp"
#include "utils/time.hpp"

#include <Core/ISceneManager.h>
#include <Core/ISceneNode.h>

#include <unordered_map>
#include <functional>

template<typename T>
struct InstanceFiller
{
    static void add(GLMesh *, scene::ISceneNode *, T &);
};

template<>
void InstanceFiller<InstanceDataShadow>::add(GLMesh *mesh, scene::ISceneNode *node, InstanceDataShadow &Instance)
{
    const core::matrix4 &mat = node->getAbsoluteTransformation();
    const core::vector3df &Origin = mat.getTranslation();
    const core::vector3df &Orientation = mat.getRotationDegrees();
    const core::vector3df &Scale = mat.getScale();
    Instance.Origin.X = Origin.X;
    Instance.Origin.Y = Origin.Y;
    Instance.Origin.Z = Origin.Z;
    Instance.Orientation.X = Orientation.X;
    Instance.Orientation.Y = Orientation.Y;
    Instance.Orientation.Z = Orientation.Z;
    Instance.Scale.X = Scale.X;
    Instance.Scale.Y = Scale.Y;
    Instance.Scale.Z = Scale.Z;
    Instance.Texture = mesh->TextureHandles[0];
}

template<>
void InstanceFiller<InstanceDataRSM>::add(GLMesh *mesh, scene::ISceneNode *node, InstanceDataRSM &Instance)
{
    const core::matrix4 &mat = node->getAbsoluteTransformation();
    const core::vector3df &Origin = mat.getTranslation();
    const core::vector3df &Orientation = mat.getRotationDegrees();
    const core::vector3df &Scale = mat.getScale();
    Instance.Origin.X = Origin.X;
    Instance.Origin.Y = Origin.Y;
    Instance.Origin.Z = Origin.Z;
    Instance.Orientation.X = Orientation.X;
    Instance.Orientation.Y = Orientation.Y;
    Instance.Orientation.Z = Orientation.Z;
    Instance.Scale.X = Scale.X;
    Instance.Scale.Y = Scale.Y;
    Instance.Scale.Z = Scale.Z;
    Instance.Texture = mesh->TextureHandles[0];
}

template<>
void InstanceFiller<InstanceDataDualTex>::add(GLMesh *mesh, scene::ISceneNode *node, InstanceDataDualTex &Instance)
{
    const core::matrix4 &mat = node->getAbsoluteTransformation();
    const core::vector3df &Origin = mat.getTranslation();
    const core::vector3df &Orientation = mat.getRotationDegrees();
    const core::vector3df &Scale = mat.getScale();
    Instance.Origin.X = Origin.X;
    Instance.Origin.Y = Origin.Y;
    Instance.Origin.Z = Origin.Z;
    Instance.Orientation.X = Orientation.X;
    Instance.Orientation.Y = Orientation.Y;
    Instance.Orientation.Z = Orientation.Z;
    Instance.Scale.X = Scale.X;
    Instance.Scale.Y = Scale.Y;
    Instance.Scale.Z = Scale.Z;
    Instance.Texture = mesh->TextureHandles[0];
    Instance.SecondTexture = mesh->TextureHandles[1];
}

template<>
void InstanceFiller<InstanceDataThreeTex>::add(GLMesh *mesh, scene::ISceneNode *node, InstanceDataThreeTex &Instance)
{
    const core::matrix4 &mat = node->getAbsoluteTransformation();
    const core::vector3df &Origin = mat.getTranslation();
    const core::vector3df &Orientation = mat.getRotationDegrees();
    const core::vector3df &Scale = mat.getScale();
    Instance.Origin.X = Origin.X;
    Instance.Origin.Y = Origin.Y;
    Instance.Origin.Z = Origin.Z;
    Instance.Orientation.X = Orientation.X;
    Instance.Orientation.Y = Orientation.Y;
    Instance.Orientation.Z = Orientation.Z;
    Instance.Scale.X = Scale.X;
    Instance.Scale.Y = Scale.Y;
    Instance.Scale.Z = Scale.Z;
    Instance.Texture = mesh->TextureHandles[0];
    Instance.SecondTexture = mesh->TextureHandles[1];
    Instance.ThirdTexture = mesh->TextureHandles[2];
}

template<>
void InstanceFiller<GlowInstanceData>::add(GLMesh *mesh, scene::ISceneNode *node, GlowInstanceData &Instance)
{
    STKMeshSceneNode *nd = dynamic_cast<STKMeshSceneNode*>(node);
    const core::matrix4 &mat = node->getAbsoluteTransformation();
    const core::vector3df &Origin = mat.getTranslation();
    const core::vector3df &Orientation = mat.getRotationDegrees();
    const core::vector3df &Scale = mat.getScale();
    Instance.Color = nd->getGlowColor().color;
    Instance.Origin.X = Origin.X;
    Instance.Origin.Y = Origin.Y;
    Instance.Origin.Z = Origin.Z;
    Instance.Orientation.X = Orientation.X;
    Instance.Orientation.Y = Orientation.Y;
    Instance.Orientation.Z = Orientation.Z;
    Instance.Scale.X = Scale.X;
    Instance.Scale.Y = Scale.Y;
    Instance.Scale.Z = Scale.Z;
}

template<typename T>
static void
FillInstances_impl(std::vector<std::pair<GLMesh *, scene::ISceneNode *> > InstanceList, T * InstanceBuffer, DrawElementsIndirectCommand *CommandBuffer,
    size_t &InstanceBufferOffset, size_t &CommandBufferOffset, size_t &PolyCount)
{
    // Should never be empty
    GLMesh *mesh = InstanceList.front().first;
    size_t InitialOffset = InstanceBufferOffset;

    for (unsigned i = 0; i < InstanceList.size(); i++)
    {
        auto &Tp = InstanceList[i];
        scene::ISceneNode *node = Tp.second;
        InstanceFiller<T>::add(mesh, node, InstanceBuffer[InstanceBufferOffset++]);
        assert(InstanceBufferOffset * sizeof(T) < 10000 * sizeof(InstanceDataDualTex));
    }

    DrawElementsIndirectCommand &CurrentCommand = CommandBuffer[CommandBufferOffset++];
    CurrentCommand.baseVertex = mesh->vaoBaseVertex;
    CurrentCommand.count = mesh->IndexCount;
    CurrentCommand.firstIndex = mesh->vaoOffset / 2;
    CurrentCommand.baseInstance = InitialOffset;
    CurrentCommand.instanceCount = InstanceBufferOffset - InitialOffset;

    PolyCount += (InstanceBufferOffset - InitialOffset) * mesh->IndexCount / 3;
}

template<typename T>
static
void FillInstances(const std::unordered_map<scene::IMeshBuffer *, std::vector<std::pair<GLMesh *, scene::ISceneNode*> > > &GatheredGLMesh, std::vector<GLMesh *> &InstancedList,
    T *InstanceBuffer, DrawElementsIndirectCommand *CommandBuffer, size_t &InstanceBufferOffset, size_t &CommandBufferOffset, size_t &Polycount)
{
    auto It = GatheredGLMesh.begin(), E = GatheredGLMesh.end();
    for (; It != E; ++It)
    {
        FillInstances_impl<T>(It->second, InstanceBuffer, CommandBuffer, InstanceBufferOffset, CommandBufferOffset, Polycount);
        if (!CVS->isAZDOEnabled())
            InstancedList.push_back(It->second.front().first);
    }
}

static std::unordered_map <scene::IMeshBuffer *, std::vector<std::pair<GLMesh *, scene::ISceneNode*> > > MeshForSolidPass[Material::SHADERTYPE_COUNT], MeshForShadowPass[Material::SHADERTYPE_COUNT][4], MeshForRSM[Material::SHADERTYPE_COUNT];
static std::unordered_map <scene::IMeshBuffer *, std::vector<std::pair<GLMesh *, scene::ISceneNode*> > > MeshForGlowPass;
static std::vector <STKMeshCommon *> DeferredUpdate;

static core::vector3df windDir;

// From irrlicht code
static
bool isBoxInFrontOfPlane(const core::plane3df &plane, const core::vector3df edges[8])
{
    for (u32 j = 0; j<8; ++j)
        if (plane.classifyPointRelation(edges[j]) != core::ISREL3D_FRONT)
            return false;
    return true;
}

std::vector<float> BoundingBoxes;

static void addEdge(const core::vector3df &P0, const core::vector3df &P1)
{
    BoundingBoxes.push_back(P0.X);
    BoundingBoxes.push_back(P0.Y);
    BoundingBoxes.push_back(P0.Z);
    BoundingBoxes.push_back(P1.X);
    BoundingBoxes.push_back(P1.Y);
    BoundingBoxes.push_back(P1.Z);
}

static
bool isCulledPrecise(const scene::ICameraSceneNode *cam, const scene::ISceneNode *node)
{
    if (!cam)
        return true;
    if (!node->getAutomaticCulling())
        return false;

    const core::matrix4 &trans = node->getAbsoluteTransformation();
    const scene::SViewFrustum &frust = *cam->getViewFrustum();

    core::vector3df edges[8];
    node->getBoundingBox().getEdges(edges);
    for (unsigned i = 0; i < 8; i++)
        trans.transformVect(edges[i]);

    for (s32 i = 0; i < scene::SViewFrustum::VF_PLANE_COUNT; ++i)
        if (isBoxInFrontOfPlane(frust.planes[i], edges))
            return true;
    return false;
}

static void
handleSTKCommon(scene::ISceneNode *Node, std::vector<scene::ISceneNode *> *ImmediateDraw,
    const scene::ICameraSceneNode *cam, const scene::ICameraSceneNode *shadowcam[4], const scene::ICameraSceneNode *rsmcam,
    bool &culledforcam, bool culledforshadowcam[4], bool &culledforrsm, bool drawRSM)
{
    STKMeshCommon *node = dynamic_cast<STKMeshCommon*>(Node);
    if (!node)
        return;
    node->updateNoGL();
    DeferredUpdate.push_back(node);


    const core::matrix4 &trans = Node->getAbsoluteTransformation();

    core::vector3df edges[8];
    Node->getBoundingBox().getEdges(edges);
    for (unsigned i = 0; i < 8; i++)
        trans.transformVect(edges[i]);

    /* From irrlicht
       /3--------/7
      / |       / |
     /  |      /  |
    1---------5   |
    |  /2- - -|- -6
    | /       |  /
    |/        | /
    0---------4/
    */

    if (irr_driver->getBoundingBoxesViz())
    {
        addEdge(edges[0], edges[1]);
        addEdge(edges[1], edges[5]);
        addEdge(edges[5], edges[4]);
        addEdge(edges[4], edges[0]);
        addEdge(edges[2], edges[3]);
        addEdge(edges[3], edges[7]);
        addEdge(edges[7], edges[6]);
        addEdge(edges[6], edges[2]);
        addEdge(edges[0], edges[2]);
        addEdge(edges[1], edges[3]);
        addEdge(edges[5], edges[7]);
        addEdge(edges[4], edges[6]);
    }

    if (node->isImmediateDraw())
    {
        ImmediateDraw->push_back(Node);
        return;
    }

    culledforcam = culledforcam || isCulledPrecise(cam, Node);
    culledforrsm = culledforrsm || isCulledPrecise(rsmcam, Node);
    for (unsigned i = 0; i < 4; i++)
        culledforshadowcam[i] = culledforshadowcam[i] || isCulledPrecise(shadowcam[i], Node);

    // Transparent

    if (World::getWorld() && World::getWorld()->isFogEnabled())
    {
        const Track * const track = World::getWorld()->getTrack();

        // Todo : put everything in a ubo
        const float fogmax = track->getFogMax();
        const float startH = track->getFogStartHeight();
        const float endH = track->getFogEndHeight();
        const float start = track->getFogStart();
        const float end = track->getFogEnd();
        const video::SColor tmpcol = track->getFogColor();

        video::SColorf col(tmpcol.getRed() / 255.0f,
            tmpcol.getGreen() / 255.0f,
            tmpcol.getBlue() / 255.0f);

        for (GLMesh *mesh : node->TransparentMesh[TM_DEFAULT])
            pushVector(ListBlendTransparentFog::getInstance(), mesh, Node->getAbsoluteTransformation(), mesh->TextureMatrix,
            fogmax, startH, endH, start, end, col);
        for (GLMesh *mesh : node->TransparentMesh[TM_ADDITIVE])
            pushVector(ListAdditiveTransparentFog::getInstance(), mesh, Node->getAbsoluteTransformation(), mesh->TextureMatrix,
            fogmax, startH, endH, start, end, col);
    }
    else
    {
        for (GLMesh *mesh : node->TransparentMesh[TM_DEFAULT])
            pushVector(ListBlendTransparent::getInstance(), mesh, Node->getAbsoluteTransformation(), mesh->TextureMatrix);
        for (GLMesh *mesh : node->TransparentMesh[TM_ADDITIVE])
            pushVector(ListAdditiveTransparent::getInstance(), mesh, Node->getAbsoluteTransformation(), mesh->TextureMatrix);
    }
    for (GLMesh *mesh : node->TransparentMesh[TM_DISPLACEMENT])
        pushVector(ListDisplacement::getInstance(), mesh, Node->getAbsoluteTransformation());

    if (!culledforcam)
    {
        for (unsigned Mat = 0; Mat < Material::SHADERTYPE_COUNT; ++Mat)
        {
            if (CVS->supportsIndirectInstancingRendering())
            {
                for (GLMesh *mesh : node->MeshSolidMaterial[Mat])
                {
                    if (node->glow())
                        MeshForGlowPass[mesh->mb].emplace_back(mesh, Node);

                    if (Mat != Material::SHADERTYPE_SPLATTING && mesh->TextureMatrix.isIdentity())
                        MeshForSolidPass[Mat][mesh->mb].emplace_back(mesh, Node);
                    else
                    {
                        core::matrix4 ModelMatrix = Node->getAbsoluteTransformation(), InvModelMatrix;
                        ModelMatrix.getInverse(InvModelMatrix);
                        switch (Mat)
                        {
                        case Material::SHADERTYPE_SOLID:
                            ListMatDefault::getInstance()->SolidPass.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                            break;
                        case Material::SHADERTYPE_ALPHA_TEST:
                            ListMatAlphaRef::getInstance()->SolidPass.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                            break;
                        case Material::SHADERTYPE_SOLID_UNLIT:
                            ListMatUnlit::getInstance()->SolidPass.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                            break;
                        case Material::SHADERTYPE_SPLATTING:
                            ListMatSplatting::getInstance()->SolidPass.emplace_back(mesh, ModelMatrix, InvModelMatrix);
                            break;
                        }
                    }
                }
            }
            else
            {
                core::matrix4 ModelMatrix = Node->getAbsoluteTransformation(), InvModelMatrix;
                ModelMatrix.getInverse(InvModelMatrix);

                for (GLMesh *mesh : node->MeshSolidMaterial[Mat])
                {
                    switch (Mat)
                    {
                    case Material::SHADERTYPE_SOLID:
                        ListMatDefault::getInstance()->SolidPass.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_ALPHA_TEST:
                        ListMatAlphaRef::getInstance()->SolidPass.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_NORMAL_MAP:
                        ListMatNormalMap::getInstance()->SolidPass.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_DETAIL_MAP:
                        ListMatDetails::getInstance()->SolidPass.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_SOLID_UNLIT:
                        ListMatUnlit::getInstance()->SolidPass.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_SPHERE_MAP:
                        ListMatSphereMap::getInstance()->SolidPass.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_SPLATTING:
                        ListMatSplatting::getInstance()->SolidPass.emplace_back(mesh, ModelMatrix, InvModelMatrix);
                        break;
                    case Material::SHADERTYPE_VEGETATION:
                        ListMatGrass::getInstance()->SolidPass.emplace_back(mesh, ModelMatrix, InvModelMatrix, windDir);
                        break;
                    }
                }
            }
        }
    }
    if (!CVS->isShadowEnabled())
        return;
    for (unsigned cascade = 0; cascade < 4; ++cascade)
    {
        if (culledforshadowcam[cascade])
            continue;
        for (unsigned Mat = 0; Mat < Material::SHADERTYPE_COUNT; ++Mat)
        {
            if (CVS->supportsIndirectInstancingRendering())
            {
                for (GLMesh *mesh : node->MeshSolidMaterial[Mat])
                {
                    if (Mat != Material::SHADERTYPE_SPLATTING)
                        MeshForShadowPass[Mat][cascade][mesh->mb].emplace_back(mesh, Node);
                    else
                    {
                        core::matrix4 ModelMatrix = Node->getAbsoluteTransformation(), InvModelMatrix;
                        ModelMatrix.getInverse(InvModelMatrix);
                        ListMatSplatting::getInstance()->Shadows[cascade].emplace_back(mesh, ModelMatrix, InvModelMatrix);
                    }
                }
            }
            else
            {
                core::matrix4 ModelMatrix = Node->getAbsoluteTransformation(), InvModelMatrix;
                ModelMatrix.getInverse(InvModelMatrix);

                for (GLMesh *mesh : node->MeshSolidMaterial[Mat])
                {
                    switch (Mat)
                    {
                    case Material::SHADERTYPE_SOLID:
                        ListMatDefault::getInstance()->Shadows[cascade].emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_ALPHA_TEST:
                        ListMatAlphaRef::getInstance()->Shadows[cascade].emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_NORMAL_MAP:
                        ListMatNormalMap::getInstance()->Shadows[cascade].emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_DETAIL_MAP:
                        ListMatDetails::getInstance()->Shadows[cascade].emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_SOLID_UNLIT:
                        ListMatUnlit::getInstance()->Shadows[cascade].emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_SPHERE_MAP:
                        ListMatSphereMap::getInstance()->Shadows[cascade].emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_SPLATTING:
                        ListMatSplatting::getInstance()->Shadows[cascade].emplace_back(mesh, ModelMatrix, InvModelMatrix);
                        break;
                    case Material::SHADERTYPE_VEGETATION:
                        ListMatGrass::getInstance()->Shadows[cascade].emplace_back(mesh, ModelMatrix, InvModelMatrix, windDir);
                    }
                }
            }
        }
    }
    if (!UserConfigParams::m_gi || !drawRSM)
        return;
    if (!culledforrsm)
    {
        for (unsigned Mat = 0; Mat < Material::SHADERTYPE_COUNT; ++Mat)
        {
            if (CVS->supportsIndirectInstancingRendering())
            {
                if (Mat == Material::SHADERTYPE_SPLATTING)
                    for (GLMesh *mesh : node->MeshSolidMaterial[Mat])
                    {
                        core::matrix4 ModelMatrix = Node->getAbsoluteTransformation(), InvModelMatrix;
                        ModelMatrix.getInverse(InvModelMatrix);
                       ListMatSplatting::getInstance()->RSM.emplace_back(mesh, ModelMatrix, InvModelMatrix);
                     }
                else
                {
                    for (GLMesh *mesh : node->MeshSolidMaterial[Mat])
                        MeshForRSM[Mat][mesh->mb].emplace_back(mesh, Node);
                }
            }
            else
            {

                core::matrix4 ModelMatrix = Node->getAbsoluteTransformation(), InvModelMatrix;
                ModelMatrix.getInverse(InvModelMatrix);

                for (GLMesh *mesh : node->MeshSolidMaterial[Mat])
                {
                    switch (Mat)
                    {
                    case Material::SHADERTYPE_SOLID:
                        ListMatDefault::getInstance()->RSM.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_ALPHA_TEST:
                        ListMatAlphaRef::getInstance()->RSM.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_NORMAL_MAP:
                        ListMatNormalMap::getInstance()->RSM.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_DETAIL_MAP:
                        ListMatDetails::getInstance()->RSM.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_SOLID_UNLIT:
                        ListMatUnlit::getInstance()->RSM.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_SPHERE_MAP:
                        ListMatSphereMap::getInstance()->RSM.emplace_back(mesh, ModelMatrix, InvModelMatrix, mesh->TextureMatrix);
                        break;
                    case Material::SHADERTYPE_SPLATTING:
                        ListMatSplatting::getInstance()->RSM.emplace_back(mesh, ModelMatrix, InvModelMatrix);
                        break;
                    case Material::SHADERTYPE_VEGETATION:
                        ListMatGrass::getInstance()->RSM.emplace_back(mesh, ModelMatrix, InvModelMatrix, windDir);
                        break;
                    }
                }
            }
        }
    }
}

static void
parseSceneManager(std::list<scene::ISceneNode*> &List, std::vector<scene::ISceneNode *> *ImmediateDraw,
    const scene::ICameraSceneNode* cam, const scene::ICameraSceneNode *shadow_cam[4], const scene::ICameraSceneNode *rsmcam,
    bool culledforcam, bool culledforshadowcam[4], bool culledforrsm, bool drawRSM)
{
    std::list<scene::ISceneNode*>::iterator I = List.begin(), E = List.end();
    for (; I != E; ++I)
    {
        if (LODNode *node = dynamic_cast<LODNode *>(*I))
            node->updateVisibility();
        (*I)->updateAbsolutePosition();
        if (!(*I)->isVisible())
            continue;

        if (ParticleSystemProxy *node = dynamic_cast<ParticleSystemProxy *>(*I))
        {
            if (!isCulledPrecise(cam, *I))
                ParticlesList::getInstance()->push_back(node);
            continue;
        }

        if (STKBillboard *node = dynamic_cast<STKBillboard *>(*I))
        {
            if (!isCulledPrecise(cam, *I))
                BillBoardList::getInstance()->push_back(node);
            continue;
        }

        bool newculledforcam = culledforcam;
        bool newculledforrsm = culledforrsm;
        bool newculledforshadowcam[4] = { culledforshadowcam[0], culledforshadowcam[1], culledforshadowcam[2], culledforshadowcam[3] };

        handleSTKCommon(*I, ImmediateDraw, cam, shadow_cam, rsmcam, newculledforcam, newculledforshadowcam, newculledforrsm, drawRSM);

        parseSceneManager(const_cast<std::list<scene::ISceneNode*>& >((*I)->getChildren()), ImmediateDraw, cam, shadow_cam, rsmcam, newculledforcam, newculledforshadowcam, newculledforrsm, drawRSM);
    }
}

template<Material::ShaderType Mat, typename T> static void
GenDrawCalls(unsigned cascade, std::vector<GLMesh *> &InstancedList,
    T *InstanceBuffer, DrawElementsIndirectCommand *CommandBuffer, size_t &InstanceBufferOffset, size_t &CommandBufferOffset, size_t &PolyCount)
{
    if (CVS->supportsIndirectInstancingRendering())
        ShadowPassCmd::getInstance()->Offset[cascade][Mat] = CommandBufferOffset; // Store command buffer offset
    FillInstances<T>(MeshForShadowPass[Mat][cascade], InstancedList, InstanceBuffer, CommandBuffer, InstanceBufferOffset, CommandBufferOffset, PolyCount);
    if (CVS->isAZDOEnabled())
        ShadowPassCmd::getInstance()->Size[cascade][Mat] = CommandBufferOffset - ShadowPassCmd::getInstance()->Offset[cascade][Mat];
}

int enableOpenMP;

static void FixBoundingBoxes(scene::ISceneNode* node)
{
    for (scene::ISceneNode *child : node->getChildren())
    {
        FixBoundingBoxes(child);
        const_cast<core::aabbox3df&>(node->getBoundingBox()).addInternalBox(child->getBoundingBox());
    }
}

void IrrDriver::PrepareDrawCalls(scene::ICameraSceneNode *camnode)
{
    windDir = getWindDir();
    ListBlendTransparent::getInstance()->clear();
    ListAdditiveTransparent::getInstance()->clear();
    ListBlendTransparentFog::getInstance()->clear();
    ListAdditiveTransparentFog::getInstance()->clear();
    ListDisplacement::getInstance()->clear();

    ListMatDefault::getInstance()->clear();
    ListMatAlphaRef::getInstance()->clear();
    ListMatSphereMap::getInstance()->clear();
    ListMatDetails::getInstance()->clear();
    ListMatUnlit::getInstance()->clear();
    ListMatNormalMap::getInstance()->clear();
    ListMatGrass::getInstance()->clear();
    ListMatSplatting::getInstance()->clear();

    ImmediateDrawList::getInstance()->clear();
    BillBoardList::getInstance()->clear();
    ParticlesList::getInstance()->clear();
    ListInstancedGlow::getInstance()->clear();

    for (unsigned Mat = 0; Mat < Material::SHADERTYPE_COUNT; ++Mat)
    {
        MeshForSolidPass[Mat].clear();
        MeshForRSM[Mat].clear();
        for (unsigned i = 0; i < 4; i++)
            MeshForShadowPass[Mat][i].clear();
    }
    MeshForGlowPass.clear();
    DeferredUpdate.clear();
    std::list<scene::ISceneNode*> List = m_scene_manager->getRootSceneNode()->getChildren();

PROFILER_PUSH_CPU_MARKER("- culling", 0xFF, 0xFF, 0x0);
    for (scene::ISceneNode *child : List)
        FixBoundingBoxes(child);

    bool cam = false, rsmcam = false;
    bool shadowcam[4] = { false, false, false, false };
    const scene::ICameraSceneNode *CascedesCam[4] =
    {
        irr_driver->getCurrentView().getCascadeCamera(0),
        irr_driver->getCurrentView().getCascadeCamera(1),
        irr_driver->getCurrentView().getCascadeCamera(2),
        irr_driver->getCurrentView().getCascadeCamera(3)
    };
    parseSceneManager(List, ImmediateDrawList::getInstance(), camnode, CascedesCam, m_suncam, cam, shadowcam, rsmcam, !m_rsm_map_available);
PROFILER_POP_CPU_MARKER();

    // Add a 1 s timeout
    if (!m_sync)
        m_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    PROFILER_PUSH_CPU_MARKER("- Sync Stall", 0xFF, 0x0, 0x0);
    GLenum reason = glClientWaitSync(m_sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0);

    while (reason != GL_ALREADY_SIGNALED)
    {
        if (reason == GL_WAIT_FAILED) break;
        StkTime::sleep(1);
        reason = glClientWaitSync(m_sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
    }
    glDeleteSync(m_sync);
    PROFILER_POP_CPU_MARKER();
    /*    switch (reason)
    {
    case GL_ALREADY_SIGNALED:
    printf("Already Signaled\n");
    break;
    case GL_TIMEOUT_EXPIRED:
    printf("Timeout Expired\n");
    break;
    case GL_CONDITION_SATISFIED:
    printf("Condition Satisfied\n");
    break;
    case GL_WAIT_FAILED:
    printf("Wait Failed\n");
    break;
    }*/
    PROFILER_PUSH_CPU_MARKER("- Animations/Buffer upload", 0x0, 0x0, 0x0);
    for (unsigned i = 0; i < DeferredUpdate.size(); i++)
        DeferredUpdate[i]->updateGL();
    PROFILER_POP_CPU_MARKER();

    if (!CVS->supportsIndirectInstancingRendering())
        return;

    InstanceDataDualTex *InstanceBufferDualTex;
    InstanceDataThreeTex *InstanceBufferThreeTex;
    InstanceDataShadow *ShadowInstanceBuffer;
    InstanceDataRSM *RSMInstanceBuffer;
    GlowInstanceData *GlowInstanceBuffer;
    DrawElementsIndirectCommand *CmdBuffer;
    DrawElementsIndirectCommand *ShadowCmdBuffer;
    DrawElementsIndirectCommand *RSMCmdBuffer;
    DrawElementsIndirectCommand *GlowCmdBuffer;

    if (CVS->supportsAsyncInstanceUpload())
    {
        InstanceBufferDualTex = (InstanceDataDualTex*)VAOManager::getInstance()->getInstanceBufferPtr(InstanceTypeDualTex);
        InstanceBufferThreeTex = (InstanceDataThreeTex*)VAOManager::getInstance()->getInstanceBufferPtr(InstanceTypeThreeTex);
        ShadowInstanceBuffer = (InstanceDataShadow*)VAOManager::getInstance()->getInstanceBufferPtr(InstanceTypeShadow);
        RSMInstanceBuffer = (InstanceDataRSM*)VAOManager::getInstance()->getInstanceBufferPtr(InstanceTypeRSM);
        GlowInstanceBuffer = (GlowInstanceData*)VAOManager::getInstance()->getInstanceBufferPtr(InstanceTypeGlow);
        CmdBuffer = SolidPassCmd::getInstance()->Ptr;
        ShadowCmdBuffer = ShadowPassCmd::getInstance()->Ptr;
        GlowCmdBuffer = GlowPassCmd::getInstance()->Ptr;
        RSMCmdBuffer = RSMPassCmd::getInstance()->Ptr;
        enableOpenMP = 1;
    }
    else
        enableOpenMP = 0;

    ListInstancedMatDefault::getInstance()->clear();
    ListInstancedMatAlphaRef::getInstance()->clear();
    ListInstancedMatGrass::getInstance()->clear();
    ListInstancedMatNormalMap::getInstance()->clear();
    ListInstancedMatSphereMap::getInstance()->clear();
    ListInstancedMatDetails::getInstance()->clear();
    ListInstancedMatUnlit::getInstance()->clear();

    size_t SolidPoly = 0, ShadowPoly = 0, MiscPoly = 0;

    PROFILER_PUSH_CPU_MARKER("- Draw Command upload", 0xFF, 0x0, 0xFF);

#pragma omp parallel sections if(enableOpenMP)
    {
#pragma omp section
        {
            size_t offset = 0, current_cmd = 0;
            if (!CVS->supportsAsyncInstanceUpload())
            {
                glBindBuffer(GL_ARRAY_BUFFER, VAOManager::getInstance()->getInstanceBuffer(InstanceTypeDualTex));
                InstanceBufferDualTex = (InstanceDataDualTex*)glMapBufferRange(GL_ARRAY_BUFFER, 0, 10000 * sizeof(InstanceDataDualTex), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                glBindBuffer(GL_DRAW_INDIRECT_BUFFER, SolidPassCmd::getInstance()->drawindirectcmd);
                CmdBuffer = (DrawElementsIndirectCommand*)glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, 10000 * sizeof(DrawElementsIndirectCommand), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            }


            // Default Material
            SolidPassCmd::getInstance()->Offset[Material::SHADERTYPE_SOLID] = current_cmd;
            FillInstances(MeshForSolidPass[Material::SHADERTYPE_SOLID], ListInstancedMatDefault::getInstance()->SolidPass, InstanceBufferDualTex, CmdBuffer, offset, current_cmd, SolidPoly);
            SolidPassCmd::getInstance()->Size[Material::SHADERTYPE_SOLID] = current_cmd - SolidPassCmd::getInstance()->Offset[Material::SHADERTYPE_SOLID];
            // Alpha Ref
            SolidPassCmd::getInstance()->Offset[Material::SHADERTYPE_ALPHA_TEST] = current_cmd;
            FillInstances(MeshForSolidPass[Material::SHADERTYPE_ALPHA_TEST], ListInstancedMatAlphaRef::getInstance()->SolidPass, InstanceBufferDualTex, CmdBuffer, offset, current_cmd, SolidPoly);
            SolidPassCmd::getInstance()->Size[Material::SHADERTYPE_ALPHA_TEST] = current_cmd - SolidPassCmd::getInstance()->Offset[Material::SHADERTYPE_ALPHA_TEST];
            // Unlit
            SolidPassCmd::getInstance()->Offset[Material::SHADERTYPE_SOLID_UNLIT] = current_cmd;
            FillInstances(MeshForSolidPass[Material::SHADERTYPE_SOLID_UNLIT], ListInstancedMatUnlit::getInstance()->SolidPass, InstanceBufferDualTex, CmdBuffer, offset, current_cmd, SolidPoly);
            SolidPassCmd::getInstance()->Size[Material::SHADERTYPE_SOLID_UNLIT] = current_cmd - SolidPassCmd::getInstance()->Offset[Material::SHADERTYPE_SOLID_UNLIT];
            // Spheremap
            SolidPassCmd::getInstance()->Offset[Material::SHADERTYPE_SPHERE_MAP] = current_cmd;
            FillInstances(MeshForSolidPass[Material::SHADERTYPE_SPHERE_MAP], ListInstancedMatSphereMap::getInstance()->SolidPass, InstanceBufferDualTex, CmdBuffer, offset, current_cmd, SolidPoly);
            SolidPassCmd::getInstance()->Size[Material::SHADERTYPE_SPHERE_MAP] = current_cmd - SolidPassCmd::getInstance()->Offset[Material::SHADERTYPE_SPHERE_MAP];
            // Grass
            SolidPassCmd::getInstance()->Offset[Material::SHADERTYPE_VEGETATION] = current_cmd;
            FillInstances(MeshForSolidPass[Material::SHADERTYPE_VEGETATION], ListInstancedMatGrass::getInstance()->SolidPass, InstanceBufferDualTex, CmdBuffer, offset, current_cmd, SolidPoly);
            SolidPassCmd::getInstance()->Size[Material::SHADERTYPE_VEGETATION] = current_cmd - SolidPassCmd::getInstance()->Offset[Material::SHADERTYPE_VEGETATION];

            if (!CVS->supportsAsyncInstanceUpload())
            {
                glUnmapBuffer(GL_ARRAY_BUFFER);
                glBindBuffer(GL_ARRAY_BUFFER, VAOManager::getInstance()->getInstanceBuffer(InstanceTypeThreeTex));
                InstanceBufferThreeTex = (InstanceDataThreeTex*)glMapBufferRange(GL_ARRAY_BUFFER, 0, 10000 * sizeof(InstanceDataThreeTex), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

            }

            // Detail
            SolidPassCmd::getInstance()->Offset[Material::SHADERTYPE_DETAIL_MAP] = current_cmd;
            FillInstances(MeshForSolidPass[Material::SHADERTYPE_DETAIL_MAP], ListInstancedMatDetails::getInstance()->SolidPass, InstanceBufferThreeTex, CmdBuffer, offset, current_cmd, SolidPoly);
            SolidPassCmd::getInstance()->Size[Material::SHADERTYPE_DETAIL_MAP] = current_cmd - SolidPassCmd::getInstance()->Offset[Material::SHADERTYPE_DETAIL_MAP];
            // Normal Map
            SolidPassCmd::getInstance()->Offset[Material::SHADERTYPE_NORMAL_MAP] = current_cmd;
            FillInstances(MeshForSolidPass[Material::SHADERTYPE_NORMAL_MAP], ListInstancedMatNormalMap::getInstance()->SolidPass, InstanceBufferThreeTex, CmdBuffer, offset, current_cmd, SolidPoly);
            SolidPassCmd::getInstance()->Size[Material::SHADERTYPE_NORMAL_MAP] = current_cmd - SolidPassCmd::getInstance()->Offset[Material::SHADERTYPE_NORMAL_MAP];


            if (!CVS->supportsAsyncInstanceUpload())
            {
                glUnmapBuffer(GL_ARRAY_BUFFER);
                glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
            }
        }
#pragma omp section
        {
            size_t offset = 0, current_cmd = 0;

            if (!CVS->supportsAsyncInstanceUpload())
            {
                glBindBuffer(GL_ARRAY_BUFFER, VAOManager::getInstance()->getInstanceBuffer(InstanceTypeGlow));
                GlowInstanceBuffer = (GlowInstanceData*)glMapBufferRange(GL_ARRAY_BUFFER, 0, 10000 * sizeof(InstanceDataDualTex), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                glBindBuffer(GL_DRAW_INDIRECT_BUFFER, GlowPassCmd::getInstance()->drawindirectcmd);
                GlowCmdBuffer = (DrawElementsIndirectCommand*)glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, 10000 * sizeof(DrawElementsIndirectCommand), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            }

            // Glow
            if (CVS->supportsIndirectInstancingRendering())
                GlowPassCmd::getInstance()->Offset = offset; // Store command buffer offset

            auto It = MeshForGlowPass.begin(), E = MeshForGlowPass.end();
            for (; It != E; ++It)
            {
                size_t Polycnt = 0;
                FillInstances_impl<GlowInstanceData>(It->second, GlowInstanceBuffer, GlowCmdBuffer, offset, current_cmd, Polycnt);
                if (!CVS->isAZDOEnabled())
                    ListInstancedGlow::getInstance()->push_back(It->second.front().first);
            }

            if (CVS->isAZDOEnabled())
                GlowPassCmd::getInstance()->Size = current_cmd - GlowPassCmd::getInstance()->Offset;

            if (!CVS->supportsAsyncInstanceUpload())
            {
                glUnmapBuffer(GL_ARRAY_BUFFER);
                glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
            }
        }
#pragma omp section
        {
            irr_driver->setPhase(SHADOW_PASS);

            size_t offset = 0, current_cmd = 0;
            if (!CVS->supportsAsyncInstanceUpload())
            {
                glBindBuffer(GL_ARRAY_BUFFER, VAOManager::getInstance()->getInstanceBuffer(InstanceTypeShadow));
                ShadowInstanceBuffer = (InstanceDataShadow*)glMapBufferRange(GL_ARRAY_BUFFER, 0, 10000 * sizeof(InstanceDataDualTex), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ShadowPassCmd::getInstance()->drawindirectcmd);
                ShadowCmdBuffer = (DrawElementsIndirectCommand*)glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, 10000 * sizeof(DrawElementsIndirectCommand), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            }

            for (unsigned i = 0; i < 4; i++)
            {
                // Mat default
                GenDrawCalls<Material::SHADERTYPE_SOLID>(i, ListInstancedMatDefault::getInstance()->Shadows[i], ShadowInstanceBuffer, ShadowCmdBuffer, offset, current_cmd, ShadowPoly);
                // Mat AlphaRef
                GenDrawCalls<Material::SHADERTYPE_ALPHA_TEST>(i, ListInstancedMatAlphaRef::getInstance()->Shadows[i], ShadowInstanceBuffer, ShadowCmdBuffer, offset, current_cmd, ShadowPoly);
                // Mat Unlit
                GenDrawCalls<Material::SHADERTYPE_SOLID_UNLIT>(i, ListInstancedMatUnlit::getInstance()->Shadows[i], ShadowInstanceBuffer, ShadowCmdBuffer, offset, current_cmd, ShadowPoly);
                // Mat NormalMap
                GenDrawCalls<Material::SHADERTYPE_NORMAL_MAP>(i, ListInstancedMatNormalMap::getInstance()->Shadows[i], ShadowInstanceBuffer, ShadowCmdBuffer, offset, current_cmd, ShadowPoly);
                // Mat Spheremap
                GenDrawCalls<Material::SHADERTYPE_SPHERE_MAP>(i, ListInstancedMatSphereMap::getInstance()->Shadows[i], ShadowInstanceBuffer, ShadowCmdBuffer, offset, current_cmd, ShadowPoly);
                // Mat Detail
                GenDrawCalls<Material::SHADERTYPE_DETAIL_MAP>(i, ListInstancedMatDetails::getInstance()->Shadows[i], ShadowInstanceBuffer, ShadowCmdBuffer, offset, current_cmd, ShadowPoly);
                // Mat Grass
                GenDrawCalls<Material::SHADERTYPE_VEGETATION>(i, ListInstancedMatGrass::getInstance()->Shadows[i], ShadowInstanceBuffer, ShadowCmdBuffer, offset, current_cmd, ShadowPoly);
            }
            if (!CVS->supportsAsyncInstanceUpload())
            {
                glUnmapBuffer(GL_ARRAY_BUFFER);
                glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
            }
        }
#pragma omp section
        if (!m_rsm_map_available)
        {
            size_t offset = 0, current_cmd = 0;
            if (!CVS->supportsAsyncInstanceUpload())
            {
                glBindBuffer(GL_ARRAY_BUFFER, VAOManager::getInstance()->getInstanceBuffer(InstanceTypeRSM));
                RSMInstanceBuffer = (InstanceDataRSM*)glMapBufferRange(GL_ARRAY_BUFFER, 0, 10000 * sizeof(InstanceDataDualTex), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                glBindBuffer(GL_DRAW_INDIRECT_BUFFER, RSMPassCmd::getInstance()->drawindirectcmd);
                RSMCmdBuffer = (DrawElementsIndirectCommand*)glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, 10000 * sizeof(DrawElementsIndirectCommand), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            }

            // Default Material
            RSMPassCmd::getInstance()->Offset[Material::SHADERTYPE_SOLID] = current_cmd;
            FillInstances(MeshForRSM[Material::SHADERTYPE_SOLID], ListInstancedMatDefault::getInstance()->RSM, RSMInstanceBuffer, RSMCmdBuffer, offset, current_cmd, MiscPoly);
            RSMPassCmd::getInstance()->Size[Material::SHADERTYPE_SOLID] = current_cmd - RSMPassCmd::getInstance()->Offset[Material::SHADERTYPE_SOLID];
            // Alpha Ref
            RSMPassCmd::getInstance()->Offset[Material::SHADERTYPE_ALPHA_TEST] = current_cmd;
            FillInstances(MeshForRSM[Material::SHADERTYPE_ALPHA_TEST], ListInstancedMatAlphaRef::getInstance()->RSM, RSMInstanceBuffer, RSMCmdBuffer, offset, current_cmd, MiscPoly);
            RSMPassCmd::getInstance()->Size[Material::SHADERTYPE_ALPHA_TEST] = current_cmd - RSMPassCmd::getInstance()->Offset[Material::SHADERTYPE_ALPHA_TEST];
            // Unlit
            RSMPassCmd::getInstance()->Offset[Material::SHADERTYPE_SOLID_UNLIT] = current_cmd;
            FillInstances(MeshForRSM[Material::SHADERTYPE_SOLID_UNLIT], ListInstancedMatUnlit::getInstance()->RSM, RSMInstanceBuffer, RSMCmdBuffer, offset, current_cmd, MiscPoly);
            RSMPassCmd::getInstance()->Size[Material::SHADERTYPE_SOLID_UNLIT] = current_cmd - RSMPassCmd::getInstance()->Offset[Material::SHADERTYPE_SOLID_UNLIT];
            // Detail
            RSMPassCmd::getInstance()->Offset[Material::SHADERTYPE_DETAIL_MAP] = current_cmd;
            FillInstances(MeshForRSM[Material::SHADERTYPE_DETAIL_MAP], ListInstancedMatDetails::getInstance()->RSM, RSMInstanceBuffer, RSMCmdBuffer, offset, current_cmd, MiscPoly);
            RSMPassCmd::getInstance()->Size[Material::SHADERTYPE_DETAIL_MAP] = current_cmd - RSMPassCmd::getInstance()->Offset[Material::SHADERTYPE_DETAIL_MAP];
            // Normal Map
            RSMPassCmd::getInstance()->Offset[Material::SHADERTYPE_NORMAL_MAP] = current_cmd;
            FillInstances(MeshForRSM[Material::SHADERTYPE_NORMAL_MAP], ListInstancedMatNormalMap::getInstance()->RSM, RSMInstanceBuffer, RSMCmdBuffer, offset, current_cmd, MiscPoly);
            RSMPassCmd::getInstance()->Size[Material::SHADERTYPE_NORMAL_MAP] = current_cmd - RSMPassCmd::getInstance()->Offset[Material::SHADERTYPE_NORMAL_MAP];

            if (!CVS->supportsAsyncInstanceUpload())
            {
                glUnmapBuffer(GL_ARRAY_BUFFER);
                glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
            }
        }
    }
    PROFILER_POP_CPU_MARKER();
    poly_count[GBUFFER_PASS] += SolidPoly;
    poly_count[SHADOW_PASS] += ShadowPoly;

    if (CVS->supportsAsyncInstanceUpload())
        glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
}