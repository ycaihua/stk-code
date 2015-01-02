#include "vaomanager.hpp"
#include "irr_driver.hpp"
#include "stkmesh.hpp"
#include "glwrap.hpp"
#include "central_settings.hpp"

VAOManager::VAOManager()
{
    for (unsigned i = 0; i < VTXTYPE_COUNT; i++)
    {
        vao[i] = 0;
        vbo[i] = 0;
        ibo[i] = 0;
        last_vertex[i] = 0;
        last_index[i] = 0;
        RealVBOSize[i] = 0;
        RealIBOSize[i] = 0;
    }
    skinning_info_vbo = 0;
    RealSkinningInfoSize = 0;

    for (unsigned i = 0; i < InstanceTypeCount; i++)
    {
        glGenBuffers(1, &instance_vbo[i]);
        glBindBuffer(GL_ARRAY_BUFFER, instance_vbo[i]);
        if (CVS->isARBBufferStorageUsable())
        {
            glBufferStorage(GL_ARRAY_BUFFER, 10000 * sizeof(InstanceDataDualTex), 0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
            Ptr[i] = glMapBufferRange(GL_ARRAY_BUFFER, 0, 10000 * sizeof(InstanceDataDualTex), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
        }
        else
        {
            glBufferData(GL_ARRAY_BUFFER, 10000 * sizeof(InstanceDataDualTex), 0, GL_STREAM_DRAW);
        }
    }
}

void VAOManager::cleanInstanceVAOs()
{
    std::map<std::pair<VTXTYPE, InstanceType>, GLuint>::iterator It = InstanceVAO.begin(), E = InstanceVAO.end();
    for (; It != E; It++)
        glDeleteVertexArrays(1, &(It->second));
    InstanceVAO.clear();
}

VAOManager::~VAOManager()
{
    cleanInstanceVAOs();
    for (unsigned i = 0; i < VTXTYPE_COUNT; i++)
    {
        if (vbo[i])
            glDeleteBuffers(1, &vbo[i]);
        if (ibo[i])
            glDeleteBuffers(1, &ibo[i]);
        if (vao[i])
            glDeleteVertexArrays(1, &vao[i]);
    }
    if (skinning_info_vbo)
        glDeleteBuffers(1, &skinning_info_vbo);
    for (unsigned i = 0; i < InstanceTypeCount; i++)
    {
        glDeleteBuffers(1, &instance_vbo[i]);
    }

}

static void
resizeBufferIfNecessary(size_t &lastIndex, size_t newLastIndex, size_t bufferSize, size_t stride, GLenum type, GLuint &id)
{
    if (newLastIndex * stride >= bufferSize)
    {
        while (newLastIndex >= bufferSize)
            bufferSize = 2 * bufferSize + 1;
        GLuint newVBO;
        glGenBuffers(1, &newVBO);
        glBindBuffer(type, newVBO);
        glBufferData(type, bufferSize * stride, 0, GL_DYNAMIC_DRAW);

        if (id)
        {
            // Copy old data
            GLuint oldVBO = id;
            glBindBuffer(GL_COPY_WRITE_BUFFER, newVBO);
            glBindBuffer(GL_COPY_READ_BUFFER, oldVBO);
            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, lastIndex * stride);
            glDeleteBuffers(1, &oldVBO);
        }
        id = newVBO;
    }
    lastIndex = newLastIndex;
}

void VAOManager::regenerateBuffer(enum VTXTYPE tp, size_t newlastvertex, size_t newlastindex)
{
    glBindVertexArray(0);
    resizeBufferIfNecessary(last_vertex[tp], newlastvertex, RealVBOSize[tp], getVertexPitch(tp), GL_ARRAY_BUFFER, vbo[tp]);
    resizeBufferIfNecessary(last_index[tp], newlastindex, RealIBOSize[tp], sizeof(u16), GL_ELEMENT_ARRAY_BUFFER, ibo[tp]);
}

void VAOManager::regenerateVAO(enum VTXTYPE tp)
{
    if (vao[tp])
        glDeleteVertexArrays(1, &vao[tp]);
    glGenVertexArrays(1, &vao[tp]);
    glBindVertexArray(vao[tp]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[tp]);

    VertexUtils::bindVertexArrayAttrib(getVertexType(tp));
    if (tp == VTXTYPE_STANDARD_SKINNED)
    {
        glBindBuffer(GL_ARRAY_BUFFER, skinning_info_vbo);
        glEnableVertexAttribArray(7);
        glVertexAttribIPointer(7, 1, GL_INT, 4 * 2 * sizeof(float), 0);
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, 4 * 2 * sizeof(float), (GLvoid*) (sizeof(int)));
        glEnableVertexAttribArray(9);
        glVertexAttribIPointer(9, 1, GL_INT, 4 * 2 * sizeof(float), (GLvoid*)(sizeof(float) + sizeof(int)));
        glEnableVertexAttribArray(10);
        glVertexAttribPointer(10, 1, GL_FLOAT, GL_FALSE, 4 * 2 * sizeof(float), (GLvoid*)(sizeof(float) + 2 * sizeof(int)));
        glEnableVertexAttribArray(11);
        glVertexAttribIPointer(11, 1, GL_INT, 4 * 2 * sizeof(float), (GLvoid*)(2 * sizeof(float) + 2 * sizeof(int)));
        glEnableVertexAttribArray(12);
        glVertexAttribPointer(12, 1, GL_FLOAT, GL_FALSE, 4 * 2 * sizeof(float), (GLvoid*)(2 * sizeof(float) + 3 * sizeof(int)));
        glEnableVertexAttribArray(13);
        glVertexAttribIPointer(13, 1, GL_INT, 4 * 2 * sizeof(float), (GLvoid*)(3 * sizeof(float) + 3 * sizeof(int)));
        glEnableVertexAttribArray(14);
        glVertexAttribPointer(14, 1, GL_FLOAT, GL_FALSE, 4 * 2 * sizeof(float), (GLvoid*)(3 * sizeof(float) + 4 * sizeof(int)));
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[tp]);
    glBindVertexArray(0);
}

template<typename T>
struct VAOInstanceUtil
{
    static void SetVertexAttrib_impl()
    {
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(T), 0);
        glVertexAttribDivisorARB(7, 1);
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, sizeof(T), (GLvoid*)(3 * sizeof(float)));
        glVertexAttribDivisorARB(8, 1);
        glEnableVertexAttribArray(9);
        glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, sizeof(T), (GLvoid*)(6 * sizeof(float)));
        glVertexAttribDivisorARB(9, 1);
    }

    static void SetVertexAttrib();
};

template<>
void VAOInstanceUtil<InstanceDataSingleTex>::SetVertexAttrib()
{
    SetVertexAttrib_impl();
    glEnableVertexAttribArray(10);
    glVertexAttribIPointer(10, 2, GL_UNSIGNED_INT, sizeof(InstanceDataSingleTex), (GLvoid*)(9 * sizeof(float)));
    glVertexAttribDivisorARB(10, 1);
}

template<>
void VAOInstanceUtil<InstanceDataDualTex>::SetVertexAttrib()
{
    SetVertexAttrib_impl();
    glEnableVertexAttribArray(10);
    glVertexAttribIPointer(10, 2, GL_UNSIGNED_INT, sizeof(InstanceDataDualTex), (GLvoid*)(9 * sizeof(float)));
    glVertexAttribDivisorARB(10, 1);
    glEnableVertexAttribArray(11);
    glVertexAttribIPointer(11, 2, GL_UNSIGNED_INT, sizeof(InstanceDataDualTex), (GLvoid*)(9 * sizeof(float) + 2 * sizeof(unsigned)));
    glVertexAttribDivisorARB(11, 1);
}

template<>
void VAOInstanceUtil<InstanceDataThreeTex>::SetVertexAttrib()
{
    SetVertexAttrib_impl();
    glEnableVertexAttribArray(10);
    glVertexAttribIPointer(10, 2, GL_UNSIGNED_INT, sizeof(InstanceDataThreeTex), (GLvoid*)(9 * sizeof(float)));
    glVertexAttribDivisorARB(10, 1);
    glEnableVertexAttribArray(11);
    glVertexAttribIPointer(11, 2, GL_UNSIGNED_INT, sizeof(InstanceDataThreeTex), (GLvoid*)(9 * sizeof(float) + 2 * sizeof(unsigned)));
    glVertexAttribDivisorARB(11, 1);
    glEnableVertexAttribArray(13);
    glVertexAttribIPointer(13, 2, GL_UNSIGNED_INT, sizeof(InstanceDataThreeTex), (GLvoid*)(9 * sizeof(float) + 4 * sizeof(unsigned)));
    glVertexAttribDivisorARB(13, 1);
}

template<>
void VAOInstanceUtil<GlowInstanceData>::SetVertexAttrib()
{
    SetVertexAttrib_impl();
    glEnableVertexAttribArray(12);
    glVertexAttribPointer(12, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GlowInstanceData), (GLvoid*)(9 * sizeof(float)));
    glVertexAttribDivisorARB(12, 1);
}


void VAOManager::regenerateInstancedVAO()
{
    cleanInstanceVAOs();

    enum video::E_VERTEX_TYPE IrrVT[] = { video::EVT_STANDARD, video::EVT_STANDARD, video::EVT_2TCOORDS, video::EVT_TANGENTS };
    for (unsigned i = 0; i < VTXTYPE_COUNT; i++)
    {
        video::E_VERTEX_TYPE tp = IrrVT[i];
        if (!vbo[i] || !ibo[i])
            continue;
        GLuint vao = createVAO(vbo[i], ibo[i], tp);
        glBindBuffer(GL_ARRAY_BUFFER, instance_vbo[InstanceTypeDualTex]);
        VAOInstanceUtil<InstanceDataDualTex>::SetVertexAttrib();
        InstanceVAO[std::pair<VTXTYPE, InstanceType>((VTXTYPE)i, InstanceTypeDualTex)] = vao;

        vao = createVAO(vbo[i], ibo[i], tp);
        glBindBuffer(GL_ARRAY_BUFFER, instance_vbo[InstanceTypeThreeTex]);
        VAOInstanceUtil<InstanceDataThreeTex>::SetVertexAttrib();
        InstanceVAO[std::pair<VTXTYPE, InstanceType>((VTXTYPE)i, InstanceTypeThreeTex)] = vao;

        vao = createVAO(vbo[i], ibo[i], tp);
        glBindBuffer(GL_ARRAY_BUFFER, instance_vbo[InstanceTypeShadow]);
        VAOInstanceUtil<InstanceDataSingleTex>::SetVertexAttrib();
        InstanceVAO[std::pair<VTXTYPE, InstanceType>((VTXTYPE)i, InstanceTypeShadow)] = vao;

        vao = createVAO(vbo[i], ibo[i], tp);
        glBindBuffer(GL_ARRAY_BUFFER, instance_vbo[InstanceTypeRSM]);
        VAOInstanceUtil<InstanceDataSingleTex>::SetVertexAttrib();
        InstanceVAO[std::pair<VTXTYPE, InstanceType>((VTXTYPE)i, InstanceTypeRSM)] = vao;

        vao = createVAO(vbo[i], ibo[i], tp);
        glBindBuffer(GL_ARRAY_BUFFER, instance_vbo[InstanceTypeGlow]);
        VAOInstanceUtil<GlowInstanceData>::SetVertexAttrib();
        InstanceVAO[std::pair<VTXTYPE, InstanceType>((VTXTYPE)i, InstanceTypeGlow)] = vao;

        glBindVertexArray(0);
    }
}

size_t VAOManager::getVertexPitch(enum VTXTYPE tp) const
{
    switch (tp)
    {
    case VTXTYPE_STANDARD:
    case VTXTYPE_STANDARD_SKINNED:
        return getVertexPitchFromType(video::EVT_STANDARD);
    case VTXTYPE_TCOORD:
        return getVertexPitchFromType(video::EVT_2TCOORDS);
    case VTXTYPE_TANGENT:
        return getVertexPitchFromType(video::EVT_TANGENTS);
    default:
        assert(0 && "Wrong vtxtype");
        return -1;
    }
}

VAOManager::VTXTYPE VAOManager::getVTXTYPE(video::E_VERTEX_TYPE type, bool skinned)
{
    if (skinned)
        return VTXTYPE_STANDARD_SKINNED;
    switch (type)
    {
    default:
        assert(0 && "Wrong vtxtype");
    case video::EVT_STANDARD:
        return VTXTYPE_STANDARD;
    case video::EVT_2TCOORDS:
        return VTXTYPE_TCOORD;
    case video::EVT_TANGENTS:
        return VTXTYPE_TANGENT;
    }
};

irr::video::E_VERTEX_TYPE VAOManager::getVertexType(enum VTXTYPE tp)
{
    switch (tp)
    {
    default:
    case VTXTYPE_STANDARD:
        return video::EVT_STANDARD;
    case VTXTYPE_TCOORD:
        return video::EVT_2TCOORDS;
    case VTXTYPE_TANGENT:
        return video::EVT_TANGENTS;
    }
}

void VAOManager::append(scene::IMeshBuffer *mb, void *SkinnedData, VTXTYPE tp)
{
    size_t old_vtx_cnt = last_vertex[tp];
    size_t old_idx_cnt = last_index[tp];

    regenerateBuffer(tp, old_vtx_cnt + mb->getVertexCount(), old_idx_cnt + mb->getIndexCount());

    glBindBuffer(GL_ARRAY_BUFFER, vbo[tp]);
    glBufferSubData(GL_ARRAY_BUFFER, old_vtx_cnt * getVertexPitch(tp), mb->getVertexCount() * getVertexPitch(tp), mb->getVertices());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo[tp]);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, old_idx_cnt * sizeof(u16), mb->getIndexCount() * sizeof(u16), mb->getIndices());

    if (!!SkinnedData)
    {
        size_t tmp = old_vtx_cnt;
        resizeBufferIfNecessary(tmp, old_vtx_cnt + mb->getVertexCount(), RealSkinningInfoSize, 4 * 2 * sizeof(float), GL_ARRAY_BUFFER, skinning_info_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, skinning_info_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, old_vtx_cnt * 4 * 2 * sizeof(float), mb->getVertexCount() * 4 * 2 * sizeof(float), SkinnedData);
    }

    mappedBaseVertex[tp][mb] = old_vtx_cnt;
    mappedBaseIndex[tp][mb] = old_idx_cnt * sizeof(u16);
}

std::pair<unsigned, unsigned> VAOManager::getBase(scene::IMeshBuffer *mb, void *Weights)
{
    VTXTYPE tp = getVTXTYPE(mb->getVertexType(), (!!Weights));
    if (mappedBaseVertex[tp].find(mb) == mappedBaseVertex[tp].end())
    {
        assert(mappedBaseIndex[tp].find(mb) == mappedBaseIndex[tp].end());
        append(mb, Weights, tp);
        regenerateVAO(tp);
        regenerateInstancedVAO();
    }

    std::unordered_map<scene::IMeshBuffer*, unsigned>::iterator It;
    It = mappedBaseVertex[tp].find(mb);
    assert(It != mappedBaseVertex[tp].end());
    unsigned vtx = It->second;
    It = mappedBaseIndex[tp].find(mb);
    assert(It != mappedBaseIndex[tp].end());
    return std::pair<unsigned, unsigned>(vtx, It->second);
}
