#ifndef STKVIEW_HPP
#define STKVIEW_HPP

#include <matrix4.h>
#include <vector>
using namespace irr;

class STKView
{
private:
    std::vector<core::matrix4> sun_ortho_matrix;
    core::vector2df    m_viewport_size;
    /** Matrixes */
    core::matrix4 m_ViewMatrix, m_InvViewMatrix;
    core::matrix4 m_ProjMatrix, m_InvProjMatrix;
    core::matrix4 m_ProjViewMatrix, m_InvProjViewMatrix;
    core::matrix4 m_previousProjViewMatrix;
public:
    void addShadowViewProj(const core::matrix4 &);
    const std::vector<core::matrix4> &getShadowViewProj() const;

    void setViewMatrix(core::matrix4 matrix);
    const core::matrix4 &getViewMatrix() const;
    const core::matrix4 &getInvViewMatrix() const;
    void setProjMatrix(core::matrix4 matrix);
    const core::matrix4 &getProjMatrix() const;
    const core::matrix4 &getInvProjMatrix() const;
    void genProjViewMatrix();
    const core::matrix4 & getPreviousPVMatrix();
    const core::matrix4 &getProjViewMatrix() const;
    const core::matrix4 &getInvProjViewMatrix() const;
    void setViewportSize(core::vector2df &);
    const core::vector2df &getCurrentScreenSize() const;
};

#endif