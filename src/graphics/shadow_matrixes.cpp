#include <limits>
#include <SceneNodes/ICameraSceneNode.h>
#include <Core/SViewFrustum.h>
#include "../../lib/irrlicht/source/Irrlicht/CSceneManager.h"
#include "../../lib/irrlicht/source/Irrlicht/os.h"
#include "graphics/central_settings.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/shaders.hpp"
#include "modes/world.hpp"
#include "physics/triangle_mesh.hpp"
#include "tracks/track.hpp"

#define MAX2(a, b) ((a) > (b) ? (a) : (b))
#define MIN2(a, b) ((a) > (b) ? (b) : (a))

static std::vector<vector3df>
getFrustrumVertex(const scene::SViewFrustum &frustrum)
{
    std::vector<vector3df> vectors;
    vectors.push_back(frustrum.getFarLeftDown());
    vectors.push_back(frustrum.getFarLeftUp());
    vectors.push_back(frustrum.getFarRightDown());
    vectors.push_back(frustrum.getFarRightUp());
    vectors.push_back(frustrum.getNearLeftDown());
    vectors.push_back(frustrum.getNearLeftUp());
    vectors.push_back(frustrum.getNearRightDown());
    vectors.push_back(frustrum.getNearRightUp());
    return vectors;
}

/** Given a matrix transform and a set of points returns an orthogonal projection matrix that maps coordinates of
transformed points between -1 and 1.
*  \param transform a transform matrix.
*  \param pointsInside a vector of point in 3d space.
*  \param returns the size (width, height) of shadowmap coverage
*/
static core::matrix4
getTighestFitOrthoProj(const core::matrix4 &transform, const std::vector<vector3df> &pointsInside, float &horizontal, float &vertical)
{
    float xmin = std::numeric_limits<float>::infinity();
    float xmax = -std::numeric_limits<float>::infinity();
    float ymin = std::numeric_limits<float>::infinity();
    float ymax = -std::numeric_limits<float>::infinity();
    float zmin = std::numeric_limits<float>::infinity();
    float zmax = -std::numeric_limits<float>::infinity();

    for (unsigned i = 0; i < pointsInside.size(); i++)
    {
        vector3df TransformedVector;
        transform.transformVect(TransformedVector, pointsInside[i]);
        xmin = MIN2(xmin, TransformedVector.X);
        xmax = MAX2(xmax, TransformedVector.X);
        ymin = MIN2(ymin, TransformedVector.Y);
        ymax = MAX2(ymax, TransformedVector.Y);
        zmin = MIN2(zmin, TransformedVector.Z);
        zmax = MAX2(zmax, TransformedVector.Z);
    }

    float left = xmin;
    float right = xmax;
    float up = ymin;
    float down = ymax;

    horizontal = right - left;
    vertical = down - up;

    core::matrix4 tmp_matrix;
    // Prevent Matrix without extend
    if (left == right || up == down)
        return tmp_matrix;
    tmp_matrix.buildProjectionMatrixOrthoLH(left, right,
        down, up,
        zmin - 100, zmax);
    return tmp_matrix;
}

/** Generate a perspective left handed matrix
*   \param n near
*   \param f far
*/
// http://msdn.microsoft.com/en-us/library/windows/desktop/bb205353(v=vs.85).aspx
static core::matrix4
PerspectiveMatrixLH(float left, float right, float top, float bottom, float n, float f)
{
    core::matrix4 projmat;
    float *M = projmat.pointer();
    // http://msdn.microsoft.com/en-us/library/windows/desktop/bb205353(v=vs.85).aspx
    M[0] = 2 * n / (right - left);
    M[5] = 2 * n / (top - bottom);
    M[8] = (left + right) / (left - right);
    M[9] = (top + bottom) / (bottom - top);
    M[10] = f / (f - n);
    M[11] = 1.;
    M[14] = n * f / (n - f);
    M[15] = 0.;
    return projmat;
}

// From http://advancedgraphics.marries.nl/presentationslides/11_light_space_perspective_shadow_maps.pdf
// With help from http://svn.openscenegraph.org/osg/OpenSceneGraph/trunk/src/osgShadow/LightSpacePerspectiveShadowMap.cpp too
static core::matrix4
getLightSpacePerspectiveMatrix(const scene::ICameraSceneNode &B, const core::vector3df &LightVector)
{
    const core::vector3df &ViewVector = (B.getTarget() - B.getAbsolutePosition()).normalize();
    // Light Space base
    const core::vector3df &axisY = (-LightVector).normalize();
    const core::vector3df &axisX = ViewVector.crossProduct(axisY).normalize();
    const core::vector3df &axisZ = axisY.crossProduct(axisX).normalize();
    const core::vector3df &Borig = B.getAbsolutePosition();

    std::vector<vector3df> vectors;
    vectors.push_back(B.getViewFrustum()->getFarLeftDown());
    vectors.push_back(B.getViewFrustum()->getFarLeftUp());
    vectors.push_back(B.getViewFrustum()->getFarRightDown());
    vectors.push_back(B.getViewFrustum()->getFarRightUp());
    vectors.push_back(B.getViewFrustum()->getNearLeftDown());
    vectors.push_back(B.getViewFrustum()->getNearLeftUp());
    vectors.push_back(B.getViewFrustum()->getNearRightDown());
    vectors.push_back(B.getViewFrustum()->getNearRightUp());

    float ymin = std::numeric_limits<float>::infinity(), ymax = -std::numeric_limits<float>::infinity();
    float zmin = std::numeric_limits<float>::infinity(), zmax = -std::numeric_limits<float>::infinity();
    for (const core::vector3df &V : vectors)
    {
        float Vy = V.dotProduct(axisY);
        ymin = MIN2(ymin, Vy);
        ymax = MAX2(ymax, Vy);
        float Vz = V.dotProduct(axisZ);
        zmin = MIN2(zmin, Vz);
        zmax = MAX2(zmax, Vz);
    }
    float n = B.getNearValue() + sqrt(B.getNearValue() * B.getFarValue());
    const core::vector3df PinLightSpace(Borig.dotProduct(axisX), (ymin + ymax) / 2., zmin - n);
    core::vector3df PinWorldSpace = PinLightSpace.X * axisX + PinLightSpace.Y * axisY + PinLightSpace.Z * axisZ;

    float thetaXleftmax = 0., thetaXrightmax = 0.;
    float thetaYleftmax = 0., thetaYrightmax = 0.;
    for (const core::vector3df &V : vectors)
    {
        const core::vector3df &TranslatedV = V - PinWorldSpace;
        float Vy = TranslatedV.dotProduct(axisY);
        float Vz = TranslatedV.dotProduct(axisZ);
        float Vx = TranslatedV.dotProduct(axisX);
        float thetaX = abs(Vx) / abs(Vz);
        if (Vx > 0.)
            thetaXrightmax = MAX2(thetaXrightmax, thetaX);
        else
            thetaXleftmax = MAX2(thetaXleftmax, thetaX);
        float thetaY = abs(Vy) / abs(Vz);
        if (Vy > 0.)
            thetaYrightmax = MAX2(thetaYrightmax, thetaY);
        else
            thetaYleftmax = MAX2(thetaYleftmax, thetaY);
    }

    core::matrix4 NewLightView;
    NewLightView.buildCameraLookAtMatrixLH(core::vector3df(0., 0., 0.), axisZ, axisY);
    NewLightView.transformVect(PinWorldSpace);

    core::matrix4 viewmat;
    viewmat.setTranslation(-PinWorldSpace);
    core::matrix4 projmat = PerspectiveMatrixLH(-thetaXleftmax * n, thetaXrightmax * n, thetaYrightmax * n, -thetaYleftmax * n, n, n + zmax - zmin);
    core::matrix4 swapmatrix;
    swapmatrix.buildCameraLookAtMatrixLH(core::vector3df(0., 0., 0.), core::vector3df(0., -1., 0.), core::vector3df(0., 0., 1.));
    core::matrix4 scalemat;
    scalemat.buildProjectionMatrixOrthoLH(-1., 1., 1., -1., 0., 1000.);
    return scalemat * swapmatrix * projmat * viewmat * NewLightView;
}

float shadowSplit[5] = { 1., 8., 32., 128., 256. };

struct CascadeBoundingBox
{
    int xmin;
    int xmax;
    int ymin;
    int ymax;
    int zmin;
    int zmax;
};

static size_t currentCBB = 0;
static CascadeBoundingBox *CBB[2];

struct Histogram
{
    int bin[1024];
    int mindepth;
    int maxdepth;
    int count;
};

/** Update shadowSplit values and make Cascade Bounding Box pointer valid.
* The function aunches two compute kernel that generates an histogram of the depth buffer value (between 0 and 250 with increment of 0.25)
* and get an axis aligned bounding box (from SunCamMatrix view) containing all depth buffer value.
* It also retrieves the result from the previous computations (in a Round Robin fashion) and update CBB pointer.
* \param width of the depth buffer
* \param height of the depth buffer
* TODO : The depth histogram part is commented out, needs to tweak it when I have some motivation
*/
void IrrDriver::UpdateSplitAndLightcoordRangeFromComputeShaders(size_t width, size_t height)
{
    // Value that should be kept between multiple calls
    static GLuint ssbo[2];
    static Histogram *Hist[2];
    static GLsync LightcoordBBFence = 0;
    static size_t currentHist = 0;
    static GLuint ssboSplit[2];
    static float tmpshadowSplit[5] = { 1., 5., 20., 50., 150. };

    if (!LightcoordBBFence)
    {
        glGenBuffers(2, ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(CascadeBoundingBox), 0, GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
        CBB[0] = (CascadeBoundingBox *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizeof(CascadeBoundingBox), GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(CascadeBoundingBox), 0, GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
        CBB[1] = (CascadeBoundingBox *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizeof(CascadeBoundingBox), GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

        /*        glGenBuffers(2, ssboSplit);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboSplit[0]);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(Histogram), 0, GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
        Hist[0] = (Histogram *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Histogram), GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboSplit[1]);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(Histogram), 0, GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
        Hist[1] = (Histogram *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Histogram), GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);*/
    }

    // Use bounding boxes from last frame
    if (LightcoordBBFence)
    {
        while (glClientWaitSync(LightcoordBBFence, GL_SYNC_FLUSH_COMMANDS_BIT, 0) != GL_ALREADY_SIGNALED);
        glDeleteSync(LightcoordBBFence);
    }

    /*    {
    memcpy(shadowSplit, tmpshadowSplit, 5 * sizeof(float));
    unsigned numpix = Hist[currentHist]->count;
    unsigned split = 0;
    unsigned i;
    for (i = 0; i < 1022; i++)
    {
    split += Hist[currentHist]->bin[i];
    if (split > numpix / 2)
    break;
    }
    tmpshadowSplit[1] = (float)++i / 4.;

    for (; i < 1023; i++)
    {
    split += Hist[currentHist]->bin[i];
    if (split > 3 * numpix / 4)
    break;
    }
    tmpshadowSplit[2] = (float)++i / 4.;

    for (; i < 1024; i++)
    {
    split += Hist[currentHist]->bin[i];
    if (split > 7 * numpix / 8)
    break;
    }
    tmpshadowSplit[3] = (float)++i / 4.;

    for (; i < 1024; i++)
    {
    split += Hist[currentHist]->bin[i];
    }

    tmpshadowSplit[0] = (float)(Hist[currentHist]->bin[1024] - 1) / 4.;
    tmpshadowSplit[4] = (float)(Hist[currentHist]->bin[1025] + 1) / 4.;
    printf("numpix is %d\n", numpix);
    printf("total : %d\n", split);
    printf("split 0 : %f\n", tmpshadowSplit[1]);
    printf("split 1 : %f\n", tmpshadowSplit[2]);
    printf("split 2 : %f\n", tmpshadowSplit[3]);
    printf("min %f max %f\n", tmpshadowSplit[0], tmpshadowSplit[4]);
    currentHist = (currentHist + 1) % 2;
    }*/

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo[currentCBB]);
    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboSplit[currentHist]);
    for (unsigned i = 0; i < 4; i++)
    {
        CBB[currentCBB][i].xmin = CBB[currentCBB][i].ymin = CBB[currentCBB][i].zmin = 1000;
        CBB[currentCBB][i].xmax = CBB[currentCBB][i].ymax = CBB[currentCBB][i].zmax = -1000;
    }
    //    memset(Hist[currentHist], 0, sizeof(Histogram));
    //    Hist[currentHist]->mindepth = 3000;
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
    glUseProgram(FullScreenShader::LightspaceBoundingBoxShader::getInstance()->Program);
    FullScreenShader::LightspaceBoundingBoxShader::getInstance()->SetTextureUnits(getDepthStencilTexture());
    FullScreenShader::LightspaceBoundingBoxShader::getInstance()->setUniforms(m_suncam->getViewMatrix(), tmpshadowSplit[1], tmpshadowSplit[2], tmpshadowSplit[3], tmpshadowSplit[4]);
    glDispatchCompute((int)width / 64, (int)height / 64, 1);

    /*    glUseProgram(FullScreenShader::DepthHistogramShader::getInstance()->Program);
    FullScreenShader::DepthHistogramShader::getInstance()->SetTextureUnits(getDepthStencilTexture());
    FullScreenShader::DepthHistogramShader::getInstance()->setUniforms();
    glDispatchCompute((int)width / 32, (int)height / 32, 1);*/

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    LightcoordBBFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    currentCBB = (currentCBB + 1) % 2;

}

/** Generate View, Projection, Inverse View, Inverse Projection, ViewProjection and InverseProjection matrixes
and matrixes and cameras for the four shadow cascade and RSM.
*   \param camnode point of view used
*   \param width of the rendering viewport
*   \param height of the rendering viewport
*/
void IrrDriver::computeMatrixesAndCameras(scene::ICameraSceneNode * const camnode, size_t width, size_t height)
{
    if (CVS->isSDSMEnabled())
        UpdateSplitAndLightcoordRangeFromComputeShaders(width, height);
    static_cast<scene::CSceneManager *>(m_scene_manager)->OnAnimate(os::Timer::getTime());
    camnode->render();
    getCurrentView().setProjMatrix(irr_driver->getVideoDriver()->getTransform(video::ETS_PROJECTION));
    getCurrentView().setViewMatrix(irr_driver->getVideoDriver()->getTransform(video::ETS_VIEW));
    getCurrentView().genProjViewMatrix();

    getCurrentView().setViewportSize(core::vector2df(float(width), float(height)));

    const float oldfar = camnode->getFarValue();
    const float oldnear = camnode->getNearValue();
    float FarValues[] =
    {
        shadowSplit[1],
        shadowSplit[2],
        shadowSplit[3],
        shadowSplit[4],
    };
    float NearValues[] =
    {
        shadowSplit[0],
        shadowSplit[1],
        shadowSplit[2],
        shadowSplit[3]
    };

    float tmp[16 * 9 + 2];
    memcpy(tmp, getCurrentView().getViewMatrix().pointer(), 16 * sizeof(float));
    memcpy(&tmp[16], getCurrentView().getProjMatrix().pointer(), 16 * sizeof(float));
    memcpy(&tmp[32], getCurrentView().getInvViewMatrix().pointer(), 16 * sizeof(float));
    memcpy(&tmp[48], getCurrentView().getInvProjMatrix().pointer(), 16 * sizeof(float));
    memcpy(&tmp[64], getCurrentView().getProjViewMatrix().pointer(), 16 * sizeof(float));

    m_suncam->render();
    const core::vector3df &LightVector = (m_suncam->getTarget() - m_suncam->getAbsolutePosition()).normalize();

    const core::matrix4 &SunCamViewMatrix = m_suncam->getViewMatrix();

    if (World::getWorld() && World::getWorld()->getTrack())
    {
        // Compute track extent
        btVector3 btmin, btmax;
        if (World::getWorld()->getTrack()->getPtrTriangleMesh())
        {
            World::getWorld()->getTrack()->getTriangleMesh().getCollisionShape().getAabb(btTransform::getIdentity(), btmin, btmax);
        }
        const Vec3 vmin = btmin, vmax = btmax;
        core::aabbox3df trackbox(vmin.toIrrVector(), vmax.toIrrVector() -
            core::vector3df(0, 30, 0));

        // Shadow Matrixes and cameras
        for (unsigned i = 0; i < 4; i++)
        {
            core::matrix4 orthogonalProjectionEnclosingMatrix, TrueProjectionMatrix;
            float h, v;
            if (!CVS->isSDSMEnabled())
            {
                camnode->setFarValue(FarValues[i]);
                camnode->setNearValue(NearValues[i]);
                camnode->render();

                getCurrentView().addViewFrustrumCascadeIntersection(camnode->getViewFrustum(), i);


                std::vector<vector3df> vectors = getFrustrumVertex(*(camnode->getViewFrustum()));
                orthogonalProjectionEnclosingMatrix = getTighestFitOrthoProj(SunCamViewMatrix, vectors, h, v);
                if (CVS->isESMEnabled())
                {
                    TrueProjectionMatrix = orthogonalProjectionEnclosingMatrix;
                }
                else
                {
                    const core::matrix4 &LiSPM = getLightSpacePerspectiveMatrix(*camnode, LightVector);
                    SunCamViewMatrix.getInverse(TrueProjectionMatrix);
                    TrueProjectionMatrix = LiSPM * TrueProjectionMatrix;
                }
            }
            else
            {
                float left = float(CBB[currentCBB][i].xmin / 4 - 2);
                float right = float(CBB[currentCBB][i].xmax / 4 + 2);
                float up = float(CBB[currentCBB][i].ymin / 4 - 2);
                float down = float(CBB[currentCBB][i].ymax / 4 + 2);

                // Prevent Matrix without extend
                if (left != right && up != down)
                {
                    orthogonalProjectionEnclosingMatrix.buildProjectionMatrixOrthoLH(left, right,
                        down, up,
                        float(CBB[currentCBB][i].zmin / 4 - 100),
                        float(CBB[currentCBB][i].zmax / 4 + 2));
                    h = right - left;
                    v = down - up;
                }
                TrueProjectionMatrix = orthogonalProjectionEnclosingMatrix;
            }
            getCurrentView().setCascadeRelativeScale(h, v, i);
            getCurrentView().addCascadeCamera(m_suncam, orthogonalProjectionEnclosingMatrix, TrueProjectionMatrix);
        }

        // Rsm Matrix and camera
        if (!m_rsm_matrix_initialized)
        {
            if (trackbox.MinEdge.X != trackbox.MaxEdge.X &&
                trackbox.MinEdge.Y != trackbox.MaxEdge.Y &&
                // Cover the case where SunCamViewMatrix is null
                SunCamViewMatrix.getScale() != core::vector3df(0., 0., 0.))
            {
                SunCamViewMatrix.transformBoxEx(trackbox);
                core::matrix4 tmp_matrix;
                tmp_matrix.buildProjectionMatrixOrthoLH(trackbox.MinEdge.X, trackbox.MaxEdge.X,
                    trackbox.MaxEdge.Y, trackbox.MinEdge.Y,
                    30, trackbox.MaxEdge.Z);
                m_suncam->setProjectionMatrix(tmp_matrix, true);
                m_suncam->render();
            }
            rsm_matrix = getVideoDriver()->getTransform(video::ETS_PROJECTION) * getVideoDriver()->getTransform(video::ETS_VIEW);
            m_rsm_matrix_initialized = true;
            m_rsm_map_available = false;
        }
        rh_extend = core::vector3df(128, 64, 128);
        core::vector3df campos = camnode->getAbsolutePosition();
        core::vector3df translation(8 * floor(campos.X / 8), 8 * floor(campos.Y / 8), 8 * floor(campos.Z / 8));
        rh_matrix.setTranslation(translation);

        // reset normal camera
        camnode->setNearValue(oldnear);
        camnode->setFarValue(oldfar);
        camnode->render();

        size_t size = getCurrentView().getShadowViewProj().size();
        for (unsigned i = 0; i < size; i++)
            memcpy(&tmp[16 * i + 80], getCurrentView().getShadowViewProj()[i].pointer(), 16 * sizeof(float));
    }

    tmp[144] = float(width);
    tmp[145] = float(height);
    glBindBuffer(GL_UNIFORM_BUFFER, SharedObject::ViewProjectionMatrixesUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, (16 * 9 + 2) * sizeof(float), tmp);
}


