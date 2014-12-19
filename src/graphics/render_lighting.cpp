#include "graphics/irr_driver.hpp"
#include "central_settings.hpp"
#include "config/user_config.hpp"
#include "graphics/glwrap.hpp"
#include "graphics/light.hpp"
#include "graphics/post_processing.hpp"
#include "graphics/rtts.hpp"
#include "graphics/shaders.hpp"
#include "modes/world.hpp"
#include "tracks/track.hpp"
#include "utils/profiler.hpp"
#include "callbacks.hpp"

#define MAX2(a, b) ((a) > (b) ? (a) : (b))
#define MIN2(a, b) ((a) > (b) ? (b) : (a))

static LightShader::PointLightInfo PointLightsInfo[MAXLIGHT];

static void renderPointLights(unsigned count)
{
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glUseProgram(LightShader::PointLightShader::getInstance()->Program);
    glBindVertexArray(LightShader::PointLightShader::getInstance()->vao);
    glBindBuffer(GL_ARRAY_BUFFER, LightShader::PointLightShader::getInstance()->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(LightShader::PointLightInfo), PointLightsInfo);

    LightShader::PointLightShader::getInstance()->SetTextureUnits(irr_driver->getRenderTargetTexture(RTT_NORMAL_AND_DEPTH), irr_driver->getDepthStencilTexture(), irr_driver->getRenderTargetTexture(RTT_BASE_COLOR));
    LightShader::PointLightShader::getInstance()->setUniforms();

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, count);
}

unsigned IrrDriver::UpdateLightsInfo(scene::ICameraSceneNode * const camnode, float dt)
{
    const u32 lightcount = (u32)m_lights.size();
    const core::vector3df &campos = camnode->getAbsolutePosition();

    std::vector<LightNode *> BucketedLN[15];
    for (unsigned int i = 0; i < lightcount; i++)
    {
        if (!m_lights[i]->isVisible())
            continue;

        if (!m_lights[i]->isPointLight())
        {
            m_lights[i]->render();
            continue;
        }
        const core::vector3df &lightpos = (m_lights[i]->getAbsolutePosition() - campos);
        unsigned idx = (unsigned)(lightpos.getLength() / 10);
        if (idx > 14)
            idx = 14;
        BucketedLN[idx].push_back(m_lights[i]);
    }

    unsigned lightnum = 0;

    for (unsigned i = 0; i < 15; i++)
    {
        for (unsigned j = 0; j < BucketedLN[i].size(); j++)
        {
            if (++lightnum >= MAXLIGHT)
            {
                LightNode* light_node = BucketedLN[i].at(j);
                light_node->setEnergyMultiplier(0.0f);
            }
            else
            {
                LightNode* light_node = BucketedLN[i].at(j);

                float em = light_node->getEnergyMultiplier();
                if (em < 1.0f)
                {
                    light_node->setEnergyMultiplier(std::min(1.0f, em + dt));
                }

                const core::vector3df &pos = light_node->getAbsolutePosition();
                PointLightsInfo[lightnum].posX = pos.X;
                PointLightsInfo[lightnum].posY = pos.Y;
                PointLightsInfo[lightnum].posZ = pos.Z;

                PointLightsInfo[lightnum].energy = light_node->getEffectiveEnergy();

                const core::vector3df &col = light_node->getColor();
                PointLightsInfo[lightnum].red = col.X;
                PointLightsInfo[lightnum].green = col.Y;
                PointLightsInfo[lightnum].blue = col.Z;

                // Light radius
                PointLightsInfo[lightnum].radius = light_node->getRadius();
            }
        }
        if (lightnum > MAXLIGHT)
        {
            irr_driver->setLastLightBucketDistance(i * 10);
            break;
        }
    }

    lightnum++;
    return lightnum;
}

/** Upload lighting info to the dedicated uniform buffer
*/
void IrrDriver::uploadLightingData()
{
    float Lighting[40];

    // Sun
    Lighting[0] = m_sundirection.X;
    Lighting[1] = m_sundirection.Y;
    Lighting[2] = m_sundirection.Z;
    Lighting[4] = m_suncolor.getRed();
    Lighting[5] = m_suncolor.getGreen();
    Lighting[6] = m_suncolor.getBlue();
    Lighting[7] = 0.54f;

    // Fog
    if (World::getWorld() && World::getWorld()->isFogEnabled())
    {
        const Track * const track = World::getWorld()->getTrack();

        const float start = track->getFogStart();
        const video::SColor tmpcol = track->getFogColor();

        core::vector3df fogcol(tmpcol.getRed() / 255.0f,
            tmpcol.getGreen() / 255.0f,
            tmpcol.getBlue() / 255.0f);
        Lighting[8] = fogcol.X;
        Lighting[9] = fogcol.Y;
        Lighting[10] = fogcol.Z;
        Lighting[11] = 1.f / (40.f * start);
    }
    else
    {
        Lighting[8] = 0.;
        Lighting[9] = 0.;
        Lighting[10] = 0.;
        Lighting[11] = 0.;
    }

    // Diffuse IBL SH
    memcpy(&Lighting[12], blueSHCoeff, 9 * sizeof(float));
    memcpy(&Lighting[21], greenSHCoeff, 9 * sizeof(float));
    memcpy(&Lighting[30], redSHCoeff, 9 * sizeof(float));

    glBindBuffer(GL_UNIFORM_BUFFER, SharedObject::LightingDataUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 40 * sizeof(float), Lighting);
}

extern float shadowSplit[5];

void IrrDriver::renderLights(unsigned pointlightcount, bool hasShadow)
{
    //RH
    if (CVS->isGlobalIlluminationEnabled() && hasShadow)
    {
        ScopedGPUTimer timer(irr_driver->getGPUTimer(Q_RH));
        glDisable(GL_BLEND);
        m_rtts->getRH().Bind();
        glBindVertexArray(SharedObject::FullScreenQuadVAO);
        if (CVS->needRHWorkaround())
        {
            glUseProgram(FullScreenShader::NVWorkaroundRadianceHintsConstructionShader::getInstance()->Program);
            FullScreenShader::NVWorkaroundRadianceHintsConstructionShader::getInstance()->SetTextureUnits(
                m_rtts->getRSM().getRTT()[0], m_rtts->getRSM().getRTT()[1], m_rtts->getRSM().getDepthTexture());
            for (unsigned i = 0; i < 32; i++)
            {
                FullScreenShader::NVWorkaroundRadianceHintsConstructionShader::getInstance()->setUniforms(rsm_matrix, rh_matrix, rh_extend, i, irr_driver->getSunColor());
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
        else
        {
            glUseProgram(FullScreenShader::RadianceHintsConstructionShader::getInstance()->Program);
            FullScreenShader::RadianceHintsConstructionShader::getInstance()->SetTextureUnits(
                    m_rtts->getRSM().getRTT()[0],
                    m_rtts->getRSM().getRTT()[1],
                    m_rtts->getRSM().getDepthTexture()
            );
            FullScreenShader::RadianceHintsConstructionShader::getInstance()->setUniforms(rsm_matrix, rh_matrix, rh_extend, irr_driver->getSunColor());
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 32);
        }
    }

    for (unsigned i = 0; i < sun_ortho_matrix.size(); i++)
        sun_ortho_matrix[i] *= getInvViewMatrix();

    m_rtts->getFBO(FBO_COLOR_AND_SPECULAR).Bind();
    glClearColor(0., 0., 0., 0.);
    glClear(GL_COLOR_BUFFER_BIT);
    m_rtts->getFBO(FBO_COLORS).Bind();
    glDepthMask(GL_FALSE);

    if (CVS->isGlobalIlluminationEnabled() && hasShadow)
    {
        ScopedGPUTimer timer(irr_driver->getGPUTimer(Q_GI));
        m_post_processing->renderGI(rh_matrix, rh_extend, m_rtts->getRH().getRTT()[0], m_rtts->getRH().getRTT()[1], m_rtts->getRH().getRTT()[2], DFG_LUT);
    }

    // Subsurface scattering
    // Algorithm is the classic Jorge Jimenez http://www.iryoku.com/sssss/
    {
        // Note : we use stencil and read from Depth Stencil texture...
        m_rtts->getFBO(FBO_COLOR_AND_SPECULAR).Bind();
        glEnable(GL_STENCIL_TEST);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilFunc(GL_EQUAL, 2, 0xFF);
        FullScreenShader::SubsurfaceShadowedSunLightShader::getInstance()->SetTextureUnits(
            irr_driver->getRenderTargetTexture(RTT_NORMAL_AND_DEPTH),
            irr_driver->getDepthStencilTexture(),
            irr_driver->getRenderTargetTexture(RTT_BASE_COLOR),
            m_rtts->getShadowFBO().getRTT()[0],
            SkyboxSpecularProbe, DFG_LUT);
        DrawFullScreenEffect<FullScreenShader::SubsurfaceShadowedSunLightShader>(shadowSplit[1], shadowSplit[2], shadowSplit[3], shadowSplit[4]);
        renderPointLights(MIN2(pointlightcount, MAXLIGHT));

        glDisable(GL_BLEND);
        // Matte texture
        getFBO(FBO_TMP1_WITH_DS).Bind();
        glClear(GL_COLOR_BUFFER_BIT);
        DrawFullScreenEffect<FullScreenShader::WhiteScreenShader>();
        glDisable(GL_STENCIL_TEST);

        // Layer 0
        FrameBuffer::Blit(getFBO(FBO_COLORS), getFBO(FBO_SUBSURFACE_LAYER0));
        // Layer 1
        FrameBuffer::Blit(getFBO(FBO_COLORS), getFBO(FBO_SUBSURFACE_LAYER1));
        // Blur layer 1
        {

            getFBO(FBO_TMP2_WITH_DS).Bind();

            FullScreenShader::SubsurfaceGaussianVShader::getInstance()->SetTextureUnits(getFBO(FBO_SUBSURFACE_LAYER1).getRTT()[0], getDepthStencilTexture(), getFBO(FBO_TMP1_WITH_DS).getRTT()[0]);
            DrawFullScreenEffect<FullScreenShader::SubsurfaceGaussianVShader>(1.f);

            getFBO(FBO_SUBSURFACE_LAYER1).Bind();

            FullScreenShader::SubsurfaceGaussianHShader::getInstance()->SetTextureUnits(getFBO(FBO_TMP2_WITH_DS).getRTT()[0], getDepthStencilTexture(), getFBO(FBO_TMP1_WITH_DS).getRTT()[0]);
            DrawFullScreenEffect<FullScreenShader::SubsurfaceGaussianHShader>(1.f);
        }
        glEnable(GL_STENCIL_TEST);
        getFBO(FBO_COLORS).Bind();
        FullScreenShader::SubsurfaceScatteringCompositionShader::getInstance()->SetTextureUnits(getRenderTargetTexture(RTT_SUBSURFACE_LAYER0), getRenderTargetTexture(RTT_SUBSURFACE_LAYER1), getRenderTargetTexture(RTT_SPECULAR));
        DrawFullScreenEffect<FullScreenShader::SubsurfaceScatteringCompositionShader>();
    }

    {
        glStencilFunc(GL_NOTEQUAL, 2, 0xFF);
        ScopedGPUTimer timer(irr_driver->getGPUTimer(Q_ENVMAP));
        m_post_processing->renderEnvMap(blueSHCoeff, greenSHCoeff, redSHCoeff, SkyboxSpecularProbe, DFG_LUT);
    }

    // Render sunlight if and only if track supports shadow
    if (!World::getWorld() || World::getWorld()->getTrack()->hasShadows())
    {
        ScopedGPUTimer timer(irr_driver->getGPUTimer(Q_SUN));
        if (World::getWorld() && CVS->isShadowEnabled() && hasShadow)
        {
            glEnable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);
            glBlendFunc(GL_ONE, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);

            glEnable(GL_STENCIL_TEST);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

            glStencilFunc(GL_EQUAL, 0, 0xFF);
            if (CVS->isESMEnabled())
            {
                FullScreenShader::ShadowedSunLightShaderESM::getInstance()->SetTextureUnits(irr_driver->getRenderTargetTexture(RTT_NORMAL_AND_DEPTH), irr_driver->getDepthStencilTexture(), irr_driver->getRenderTargetTexture(RTT_BASE_COLOR), m_rtts->getShadowFBO().getRTT()[0]);
                DrawFullScreenEffect<FullScreenShader::ShadowedSunLightShaderESM>(shadowSplit[1], shadowSplit[2], shadowSplit[3], shadowSplit[4]);
            }
            else
            {
                FullScreenShader::ShadowedSunLightShaderPCF::getInstance()->SetTextureUnits(irr_driver->getRenderTargetTexture(RTT_NORMAL_AND_DEPTH), irr_driver->getDepthStencilTexture(), irr_driver->getRenderTargetTexture(RTT_BASE_COLOR), m_rtts->getShadowFBO().getDepthTexture());
                DrawFullScreenEffect<FullScreenShader::ShadowedSunLightShaderPCF>(shadowSplit[1], shadowSplit[2], shadowSplit[3], shadowSplit[4], UserConfigParams::m_shadows_resolution);
            }
            glStencilFunc(GL_EQUAL, 1, 0xFF);

            FullScreenShader::BacklitShadowedSunLightShader::getInstance()->SetTextureUnits(irr_driver->getRenderTargetTexture(RTT_NORMAL_AND_DEPTH), irr_driver->getDepthStencilTexture(), irr_driver->getRenderTargetTexture(RTT_BASE_COLOR), m_rtts->getShadowFBO().getRTT()[0]);
            DrawFullScreenEffect<FullScreenShader::BacklitShadowedSunLightShader>(shadowSplit[1], shadowSplit[2], shadowSplit[3], shadowSplit[4]);
            m_post_processing->renderBacklitShadowedSunlight(irr_driver->getSunDirection(), irr_driver->getSunColor(), sun_ortho_matrix, m_rtts->getShadowFBO().getRTT()[0]);
            glDisable(GL_STENCIL_TEST);
        }
        else
            m_post_processing->renderSunlight(irr_driver->getSunDirection(), irr_driver->getSunColor());
    }
    {
        ScopedGPUTimer timer(irr_driver->getGPUTimer(Q_POINTLIGHTS));
        glStencilFunc(GL_NOTEQUAL, 2, 0xFF);
        renderPointLights(MIN2(pointlightcount, MAXLIGHT));
        glDisable(GL_STENCIL_TEST);
    }
}

void IrrDriver::renderSSAO()
{
    m_rtts->getFBO(FBO_SSAO).Bind();
    glClearColor(1., 1., 1., 1.);
    glClear(GL_COLOR_BUFFER_BIT);
    m_post_processing->renderSSAO();
    // Blur it to reduce noise.
    FrameBuffer::Blit(m_rtts->getFBO(FBO_SSAO), m_rtts->getFBO(FBO_HALF1_R), GL_COLOR_BUFFER_BIT, GL_LINEAR);
    m_post_processing->renderGaussian17TapBlur(irr_driver->getFBO(FBO_HALF1_R), irr_driver->getFBO(FBO_HALF2_R));

    getFBO(FBO_COLORS).Bind();
    m_post_processing->applySSAO(getRenderTargetTexture(RTT_HALF1_R));
}

void IrrDriver::renderLightsScatter(unsigned pointlightcount)
{
    getFBO(FBO_HALF1).Bind();
    glClearColor(0., 0., 0., 0.);
    glClear(GL_COLOR_BUFFER_BIT);

    const Track * const track = World::getWorld()->getTrack();

    // This function is only called once per frame - thus no need for setters.
    float start = track->getFogStart() + .001f;
    const video::SColor tmpcol = track->getFogColor();

    core::vector3df col(tmpcol.getRed() / 255.0f,
        tmpcol.getGreen() / 255.0f,
        tmpcol.getBlue() / 255.0f);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    FullScreenShader::FogShader::getInstance()->SetTextureUnits(irr_driver->getDepthStencilTexture());
    DrawFullScreenEffect<FullScreenShader::FogShader>();

    glEnable(GL_DEPTH_TEST);

    glUseProgram(LightShader::PointLightScatterShader::getInstance()->Program);
    glBindVertexArray(LightShader::PointLightScatterShader::getInstance()->vao);

    LightShader::PointLightScatterShader::getInstance()->SetTextureUnits(irr_driver->getDepthStencilTexture());
    LightShader::PointLightScatterShader::getInstance()->setUniforms();

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, MIN2(pointlightcount, MAXLIGHT));

    glDisable(GL_BLEND);
    m_post_processing->renderGaussian6Blur(getFBO(FBO_HALF1), getFBO(FBO_HALF2), 5., 5.);
    glEnable(GL_BLEND);

    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    getFBO(FBO_COLORS).Bind();
    m_post_processing->renderPassThrough(getRenderTargetTexture(RTT_HALF1));
}
