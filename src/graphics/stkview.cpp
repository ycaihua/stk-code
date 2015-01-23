#include "stkview.hpp"
#include <cassert>


void STKView::setViewMatrix(core::matrix4 matrix)
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

void STKView::setProjMatrix(core::matrix4 matrix)
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
}

void STKView::addShadowViewProj(const core::matrix4 &m)
{
    sun_ortho_matrix.push_back(m);
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

void STKView::setViewportSize(core::vector2df &v)
{
    m_viewport_size = v;
}

const core::vector2df &STKView::getCurrentScreenSize() const
{
    return m_viewport_size;
}