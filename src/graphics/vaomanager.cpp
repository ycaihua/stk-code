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

    for (unsigned i = 0; i < InstanceTypeCount; i++)
    {
        glGenBuffers(1, &instance_vbo[i]);
        glBindBuffer(GL_ARRAY_BUFFER, instance_vbo[i]);
        if (CVS->supportsAsyncInstanceUpload())
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
    std::map<std::pair<video::E_VERTEX_TYPE, InstanceType>, GLuint>::iterator It = InstanceVAO.begin(), E = InstanceVAO.end();
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
    for (unsigned i = 0; i < InstanceTypeCount; i++)
    {
        glDeleteBuffers(1, &instance_vbo[i]);
    }

}

template<>
class VertexAttribBinder<video::S3DVertex>
{
public:
    static void bind()
    {
        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(video::S3DVertex), 0);
        // Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(video::S3DVertex), (GLvoid*)12);
        // Color
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(video::S3DVertex), (GLvoid*)24);
        // Texcoord
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(video::S3DVertex), (GLvoid*)28);
    }
};

template<>
class VertexAttribBinder<video::S3DVertex2TCoords>
{
public:
    static void bind()
    {
        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(video::S3DVertex2TCoords), 0);
        // Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(video::S3DVertex2TCoords), (GLvoid*)12);
        // Color
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(video::S3DVertex2TCoords), (GLvoid*)24);
        // Texcoord
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(video::S3DVertex2TCoords), (GLvoid*)28);
        // SecondTexcoord
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(video::S3DVertex2TCoords), (GLvoid*)36);
    }
};

template<>
class VertexAttribBinder<video::S3DVertexTangents>
{
public:
    static void bind()
    {
        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(video::S3DVertexTangents), 0);
        // Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(video::S3DVertexTangents), (GLvoid*)12);
        // Color
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(video::S3DVertexTangents), (GLvoid*)24);
        // Texcoord
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(video::S3DVertexTangents), (GLvoid*)28);
        // Tangent
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(video::S3DVertexTangents), (GLvoid*)36);
        // Bitangent
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(video::S3DVertexTangents), (GLvoid*)48);
    }
};


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

template <typename VT>
static void regenerateInstanceVao(video::E_VERTEX_TYPE tp, std::map<std::pair<irr::video::E_VERTEX_TYPE, InstanceType>, GLuint> &InstanceVAO, GLuint instance_vbo[InstanceTypeCount])
{
    if (VertexArrayObject<VT>::getInstance()->isEmpty())
        return;
    GLuint vao = VertexArrayObject<VT>::getInstance()->getNewVAO();
    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo[InstanceTypeDualTex]);
    VAOInstanceUtil<InstanceDataDualTex>::SetVertexAttrib();
    InstanceVAO[std::pair<video::E_VERTEX_TYPE, InstanceType>(tp, InstanceTypeDualTex)] = vao;

    vao = VertexArrayObject<VT>::getInstance()->getNewVAO();
    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo[InstanceTypeThreeTex]);
    VAOInstanceUtil<InstanceDataThreeTex>::SetVertexAttrib();
    InstanceVAO[std::pair<video::E_VERTEX_TYPE, InstanceType>(tp, InstanceTypeThreeTex)] = vao;

    vao = VertexArrayObject<VT>::getInstance()->getNewVAO();
    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo[InstanceTypeShadow]);
    VAOInstanceUtil<InstanceDataSingleTex>::SetVertexAttrib();
    InstanceVAO[std::pair<video::E_VERTEX_TYPE, InstanceType>(tp, InstanceTypeShadow)] = vao;

    vao = VertexArrayObject<VT>::getInstance()->getNewVAO();
    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo[InstanceTypeRSM]);
    VAOInstanceUtil<InstanceDataSingleTex>::SetVertexAttrib();
    InstanceVAO[std::pair<video::E_VERTEX_TYPE, InstanceType>(tp, InstanceTypeRSM)] = vao;

    vao = VertexArrayObject<VT>::getInstance()->getNewVAO();
    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo[InstanceTypeGlow]);
    VAOInstanceUtil<GlowInstanceData>::SetVertexAttrib();
    InstanceVAO[std::pair<video::E_VERTEX_TYPE, InstanceType>(tp, InstanceTypeGlow)] = vao;

    glBindVertexArray(0);

}


void VAOManager::regenerateInstancedVAO()
{
    cleanInstanceVAOs();
    regenerateInstanceVao<video::S3DVertex>(video::EVT_STANDARD, InstanceVAO, instance_vbo);
    regenerateInstanceVao<video::S3DVertex2TCoords>(video::EVT_2TCOORDS, InstanceVAO, instance_vbo);
    regenerateInstanceVao<video::S3DVertexTangents>(video::EVT_TANGENTS, InstanceVAO, instance_vbo);
}

size_t VAOManager::getVertexPitch(enum VTXTYPE tp) const
{
    switch (tp)
    {
    case VTXTYPE_STANDARD:
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

VAOManager::VTXTYPE VAOManager::getVTXTYPE(video::E_VERTEX_TYPE type)
{
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

std::pair<unsigned, unsigned> VAOManager::getBase(scene::IMeshBuffer *mb)
{
    VTXTYPE tp = getVTXTYPE(mb->getVertexType());
    switch (tp)
    {
    default:
        assert(0 && "Wrong type");
    case video::EVT_STANDARD:
        return VertexArrayObject<video::S3DVertex>::getInstance()->getBase(mb);
    case video::EVT_2TCOORDS:
        return VertexArrayObject<video::S3DVertex2TCoords>::getInstance()->getBase(mb);
    case video::EVT_TANGENTS:
        return VertexArrayObject<video::S3DVertexTangents>::getInstance()->getBase(mb);
    }
}

GLuint VAOManager::getInstanceBuffer(InstanceType it)
{
    return instance_vbo[it];
}

void *VAOManager::getInstanceBufferPtr(InstanceType it)
{
    return Ptr[it];
}

unsigned VAOManager::getVBO(irr::video::E_VERTEX_TYPE type)
{
    switch (type)
    {
    default:
        assert(0 && "Wrong type");
    case video::EVT_STANDARD:
        return VertexArrayObject<video::S3DVertex>::getInstance()->vbo.getBuffer();
    case video::EVT_2TCOORDS:
        return VertexArrayObject<video::S3DVertex2TCoords>::getInstance()->vbo.getBuffer();
    case video::EVT_TANGENTS:
        return VertexArrayObject<video::S3DVertexTangents>::getInstance()->vbo.getBuffer();
    }
}

void *VAOManager::getVBOPtr(irr::video::E_VERTEX_TYPE type)
{
    switch (type)
    {
    default:
        assert(0 && "Wrong type");
    case video::EVT_STANDARD:
        return VertexArrayObject<video::S3DVertex>::getInstance()->vbo.getPointer();
    case video::EVT_2TCOORDS:
        return VertexArrayObject<video::S3DVertex2TCoords>::getInstance()->vbo.getPointer();
    case video::EVT_TANGENTS:
        return VertexArrayObject<video::S3DVertexTangents>::getInstance()->vbo.getPointer();
    }
}
unsigned VAOManager::getVAO(irr::video::E_VERTEX_TYPE type)
{
    switch (type)
    {
    default:
        assert(0 && "Wrong type");
    case video::EVT_STANDARD:
        return VertexArrayObject<video::S3DVertex>::getInstance()->getVAO();
    case video::EVT_2TCOORDS:
        return VertexArrayObject<video::S3DVertex2TCoords>::getInstance()->getVAO();
    case video::EVT_TANGENTS:
        return VertexArrayObject<video::S3DVertexTangents>::getInstance()->getVAO();
    }
}

unsigned VAOManager::getInstanceVAO(irr::video::E_VERTEX_TYPE vt, enum InstanceType it)
{
    switch (vt)
    {
    default:
        assert(0 && "Wrong type");
    case video::EVT_STANDARD:
        if (VertexArrayObject<video::S3DVertex>::getInstance()->dirty)
        {
            regenerateInstancedVAO();
            VertexArrayObject<video::S3DVertex>::getInstance()->dirty = false;
        }
        break;
    case video::EVT_2TCOORDS:
        if (VertexArrayObject<video::S3DVertex2TCoords>::getInstance()->dirty)
        {
            regenerateInstancedVAO();
            VertexArrayObject<video::S3DVertex2TCoords>::getInstance()->dirty = false;
        }
        break;
    case video::EVT_TANGENTS:
        if (VertexArrayObject<video::S3DVertexTangents>::getInstance()->dirty)
        {
            regenerateInstancedVAO();
            VertexArrayObject<video::S3DVertexTangents>::getInstance()->dirty = false;
        }
        break;
    }
    return InstanceVAO[std::pair<irr::video::E_VERTEX_TYPE, InstanceType>(vt, it)];
}