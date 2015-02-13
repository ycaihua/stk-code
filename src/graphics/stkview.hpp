#ifndef STKVIEW_HPP
#define STKVIEW_HPP

#include <Maths/matrix4.h>
#include <vector>
#include <SceneNodes/ICameraSceneNode.h>

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

    std::pair<float, float> m_shadow_scales[4];
    scene::ICameraSceneNode *m_shadow_camnodes[4];
    float m_shadows_cam[4][24];
public:
    STKView();
    ~STKView();
    void reset();

    const std::vector<core::matrix4> &getShadowViewProj() const;

    void addViewFrustrumCascadeIntersection(const scene::SViewFrustum *frustrum, unsigned cascade);
    const float *getViewFrustrumCascadeVertices(unsigned cascade) const;

    void addCascadeCamera(const scene::ICameraSceneNode *SunCamera, const core::matrix4 &ProjectionEnclosingMatrix, const core::matrix4 &TrueProjectionMatrix);
    const scene::ICameraSceneNode *getCascadeCamera(unsigned cascade) const;

    void setCascadeRelativeScale(float horizontal, float vertical, unsigned cascade);
    std::pair<float, float> getRelativeScale(unsigned cascade) const;


    void setViewMatrix(const core::matrix4 &matrix);
    const core::matrix4 &getViewMatrix() const;
    const core::matrix4 &getInvViewMatrix() const;

    void setProjMatrix(const core::matrix4 &matrix);
    const core::matrix4 &getProjMatrix() const;
    const core::matrix4 &getInvProjMatrix() const;

    void genProjViewMatrix();
    const core::matrix4 & getPreviousPVMatrix();
    const core::matrix4 &getProjViewMatrix() const;
    const core::matrix4 &getInvProjViewMatrix() const;

    void setViewportSize(const core::vector2df &);
    const core::vector2df &getCurrentScreenSize() const;
};

#endif