#include "vaomanager.hpp"
#include "irr_driver.hpp"
#include "stkmesh.hpp"
#include "glwrap.hpp"
#include "central_settings.hpp"

VAOManager::VAOManager()
{
    // Init all instance buffer
    InstanceBuffer<InstanceDataDualTex>::getInstance();
    InstanceBuffer<InstanceDataThreeTex>::getInstance();
    InstanceBuffer<InstanceDataShadow>::getInstance();
    InstanceBuffer<InstanceDataRSM>::getInstance();
    InstanceBuffer<GlowInstanceData>::getInstance();

}

VAOManager::~VAOManager()
{
    InstanceBuffer<InstanceDataDualTex>::getInstance()->kill();
    InstanceBuffer<InstanceDataThreeTex>::getInstance()->kill();
    InstanceBuffer<InstanceDataShadow>::getInstance()->kill();
    InstanceBuffer<InstanceDataRSM>::getInstance()->kill();
    InstanceBuffer<GlowInstanceData>::getInstance()->kill();
}

template<>
struct VertexAttribBinder<video::S3DVertex>
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
struct VertexAttribBinder<video::S3DVertex2TCoords>
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
struct VertexAttribBinder<video::S3DVertexTangents>
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

std::pair<unsigned, unsigned> VAOManager::getBase(scene::IMeshBuffer *mb)
{
    switch (mb->getVertexType())
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
    switch (it)
    {
    default:
        assert(0 && "wrong instance type");
    case InstanceTypeDualTex:
        return InstanceBuffer<InstanceDataDualTex>::getInstance()->getBuffer();
    case InstanceTypeThreeTex:
        return InstanceBuffer<InstanceDataThreeTex>::getInstance()->getBuffer();
    case InstanceTypeShadow:
        return InstanceBuffer<InstanceDataShadow>::getInstance()->getBuffer();
    case InstanceTypeRSM:
        return InstanceBuffer<InstanceDataRSM>::getInstance()->getBuffer();
    case InstanceTypeGlow:
        return InstanceBuffer<GlowInstanceData>::getInstance()->getBuffer();
    }
}

void *VAOManager::getInstanceBufferPtr(InstanceType it)
{
    switch (it)
    {
    default:
        assert(0 && "wrong instance type");
    case InstanceTypeDualTex:
        return InstanceBuffer<InstanceDataDualTex>::getInstance()->getPointer();
    case InstanceTypeThreeTex:
        return InstanceBuffer<InstanceDataThreeTex>::getInstance()->getPointer();
    case InstanceTypeShadow:
        return InstanceBuffer<InstanceDataShadow>::getInstance()->getPointer();
    case InstanceTypeRSM:
        return InstanceBuffer<InstanceDataRSM>::getInstance()->getPointer();
    case InstanceTypeGlow:
        return InstanceBuffer<GlowInstanceData>::getInstance()->getPointer();
    }
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
        switch (it)
        {
        default:
            assert(0 && "wrong instance type");
        case InstanceTypeDualTex:
            return VertexArrayObject<video::S3DVertex>::getInstance()->vao_instanceDualTex;
        case InstanceTypeThreeTex:
            return VertexArrayObject<video::S3DVertex>::getInstance()->vao_instanceThreeTex;
        case InstanceTypeShadow:
            return VertexArrayObject<video::S3DVertex>::getInstance()->vao_instanceShadow;
        case InstanceTypeRSM:
            return VertexArrayObject<video::S3DVertex>::getInstance()->vao_instanceRSM;
        case InstanceTypeGlow:
            return VertexArrayObject<video::S3DVertex>::getInstance()->vao_instanceGlowData;
        }
        break;
    case video::EVT_2TCOORDS:
        switch (it)
        {
        default:
            assert(0 && "wrong instance type");
        case InstanceTypeDualTex:
            return VertexArrayObject<video::S3DVertex2TCoords>::getInstance()->vao_instanceDualTex;
        case InstanceTypeThreeTex:
            return VertexArrayObject<video::S3DVertex2TCoords>::getInstance()->vao_instanceThreeTex;
        case InstanceTypeShadow:
            return VertexArrayObject<video::S3DVertex2TCoords>::getInstance()->vao_instanceShadow;
        case InstanceTypeRSM:
            return VertexArrayObject<video::S3DVertex2TCoords>::getInstance()->vao_instanceRSM;
        case InstanceTypeGlow:
            return VertexArrayObject<video::S3DVertex2TCoords>::getInstance()->vao_instanceGlowData;
        }
        break;
    case video::EVT_TANGENTS:
        switch (it)
        {
        default:
            assert(0 && "wrong instance type");
        case InstanceTypeDualTex:
            return VertexArrayObject<video::S3DVertexTangents>::getInstance()->vao_instanceDualTex;
        case InstanceTypeThreeTex:
            return VertexArrayObject<video::S3DVertexTangents>::getInstance()->vao_instanceThreeTex;
        case InstanceTypeShadow:
            return VertexArrayObject<video::S3DVertexTangents>::getInstance()->vao_instanceShadow;
        case InstanceTypeRSM:
            return VertexArrayObject<video::S3DVertexTangents>::getInstance()->vao_instanceRSM;
        case InstanceTypeGlow:
            return VertexArrayObject<video::S3DVertexTangents>::getInstance()->vao_instanceGlowData;
        }
        break;
    }
}