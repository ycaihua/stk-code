#include "stkview.hpp"
#include "irr_driver.hpp"
#include <cassert>
#include <Core/SViewFrustum.h>

STKView::STKView()
{
    reset();
}

STKView::~STKView()
{

}

void STKView::reset()
{
    memset(m_shadow_camnodes, 0, 4 * sizeof(void*));
}

void STKView::addViewFrustrumCascadeIntersection(const scene::SViewFrustum *frustrum, unsigned cascade)
{
    float tmp[24] = {
        frustrum->getFarLeftDown().X,
        frustrum->getFarLeftDown().Y,
        frustrum->getFarLeftDown().Z,
        frustrum->getFarLeftUp().X,
        frustrum->getFarLeftUp().Y,
        frustrum->getFarLeftUp().Z,
        frustrum->getFarRightDown().X,
        frustrum->getFarRightDown().Y,
        frustrum->getFarRightDown().Z,
        frustrum->getFarRightUp().X,
        frustrum->getFarRightUp().Y,
        frustrum->getFarRightUp().Z,
        frustrum->getNearLeftDown().X,
        frustrum->getNearLeftDown().Y,
        frustrum->getNearLeftDown().Z,
        frustrum->getNearLeftUp().X,
        frustrum->getNearLeftUp().Y,
        frustrum->getNearLeftUp().Z,
        frustrum->getNearRightDown().X,
        frustrum->getNearRightDown().Y,
        frustrum->getNearRightDown().Z,
        frustrum->getNearRightUp().X,
        frustrum->getNearRightUp().Y,
        frustrum->getNearRightUp().Z,
    };
    memcpy(m_shadows_cam[cascade], tmp, 24 * sizeof(float));
}

const float *STKView::getViewFrustrumCascadeVertices(unsigned cascade) const
{
    return m_shadows_cam[cascade];
}

void STKView::addCascadeCamera(const scene::ICameraSceneNode *SunCamera, const core::matrix4 &ProjectionMatrixForCulling, const core::matrix4 &TrueProjectionMatrix)
{
    unsigned i = sun_ortho_matrix.size();
    m_shadow_camnodes[i] = (scene::ICameraSceneNode *) const_cast<scene::ICameraSceneNode *>(SunCamera)->clone();

    m_shadow_camnodes[i]->setProjectionMatrix(ProjectionMatrixForCulling, true);
    m_shadow_camnodes[i]->render();
    sun_ortho_matrix.push_back(TrueProjectionMatrix * m_shadow_camnodes[i]->getViewMatrix());
}

const scene::ICameraSceneNode *STKView::getCascadeCamera(unsigned cascade) const
{
    return m_shadow_camnodes[cascade];
}

void STKView::setCascadeRelativeScale(float horizontal, float vertical, unsigned cascade)
{
    m_shadow_scales[cascade] = std::make_pair(horizontal, vertical);
}

std::pair<float, float> STKView::getRelativeScale(unsigned cascade) const
{
    return m_shadow_scales[cascade];
}

void STKView::setViewMatrix(const core::matrix4 &matrix)
{
    m_ViewMatrix = matrix;
    matrix.getInverse(m_InvViewMatrix);
}

const core::matrix4 &STKView::getViewMatrix() const
{
    return m_ViewMatrix;
}

const core::matrix4 &STKView::getInvViewMatrix() const
{
    return m_InvViewMatrix;
}

void STKView::setProjMatrix(const core::matrix4 &matrix)
{
    m_ProjMatrix = matrix;
    matrix.getInverse(m_InvProjMatrix);
}

const core::matrix4 &STKView::getProjMatrix() const
{
    return m_ProjMatrix;
}

const core::matrix4 &STKView::getInvProjMatrix() const
{
    return m_InvProjMatrix;
}

void STKView::genProjViewMatrix()
{
    m_previousProjViewMatrix = m_ProjViewMatrix;
    m_ProjViewMatrix = m_ProjMatrix * m_ViewMatrix;
    m_InvProjViewMatrix = m_ProjViewMatrix;
    m_InvProjViewMatrix.makeInverse();
    sun_ortho_matrix.clear();
    for (unsigned i = 0; i < 4; i++)
    {
        if (m_shadow_camnodes[i])
            delete m_shadow_camnodes[i];
    }
}

const std::vector<core::matrix4> &STKView::getShadowViewProj() const
{
    assert(sun_ortho_matrix.size() == 4);
    return sun_ortho_matrix;
}

const core::matrix4 &STKView::getPreviousPVMatrix()
{
    return m_previousProjViewMatrix;
}

const core::matrix4 &STKView::getProjViewMatrix() const
{
    return m_ProjViewMatrix;
}

const core::matrix4 &STKView::getInvProjViewMatrix() const
{
    return m_InvProjViewMatrix;
}

void STKView::setViewportSize(const core::vector2df &v)
{
    m_viewport_size = v;
}

const core::vector2df &STKView::getCurrentScreenSize() const
{
    return m_viewport_size;
}