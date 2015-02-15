#ifndef VAOMANAGER_HPP
#define VAOMANAGER_HPP

#include "gl_headers.hpp"
#include "utils/singleton.hpp"
#include "central_settings.hpp"
#include <Core/S3DVertex.h>
#include <Core/IMeshBuffer.h>
#include <vector>
#include <map>
#include <unordered_map>

enum InstanceType
{
    InstanceTypeDualTex,
    InstanceTypeThreeTex,
    InstanceTypeShadow,
    InstanceTypeRSM,
    InstanceTypeGlow,
    InstanceTypeCount,
};

#ifdef WIN32
#pragma pack(push, 1)
#endif
struct InstanceDataShadow
{
    struct
    {
        float X;
        float Y;
        float Z;
    } Origin;
    struct
    {
        float X;
        float Y;
        float Z;
    } Orientation;
    struct
    {
        float X;
        float Y;
        float Z;
    } Scale;
    uint64_t Texture;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

struct InstanceDataRSM
{
    struct
    {
        float X;
        float Y;
        float Z;
    } Origin;
    struct
    {
        float X;
        float Y;
        float Z;
    } Orientation;
    struct
    {
        float X;
        float Y;
        float Z;
    } Scale;
    uint64_t Texture;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

struct InstanceDataDualTex
{
    struct
    {
        float X;
        float Y;
        float Z;
    } Origin;
    struct
    {
        float X;
        float Y;
        float Z;
    } Orientation;
    struct
    {
        float X;
        float Y;
        float Z;
    } Scale;
    uint64_t Texture;
    uint64_t SecondTexture;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

struct InstanceDataThreeTex
{
    struct
    {
        float X;
        float Y;
        float Z;
    } Origin;
    struct
    {
        float X;
        float Y;
        float Z;
    } Orientation;
    struct
    {
        float X;
        float Y;
        float Z;
    } Scale;
    uint64_t Texture;
    uint64_t SecondTexture;
    uint64_t ThirdTexture;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

struct GlowInstanceData
{
    struct
    {
        float X;
        float Y;
        float Z;
    } Origin;
    struct
    {
        float X;
        float Y;
        float Z;
    } Orientation;
    struct
    {
        float X;
        float Y;
        float Z;
    } Scale;
    unsigned Color;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif
#ifdef WIN32
#pragma pack(pop)
#endif

// Array_buffer for vertex data or instances
template<typename Data>
class ArrayBuffer
{
private:
    GLuint buffer;
    size_t realSize;
    size_t advertisedSize;

    Data *Pointer;
public:
    void append(irr::scene::IMeshBuffer *);

    ArrayBuffer() : buffer(0), realSize(0), advertisedSize(0), Pointer(0) { }

    ~ArrayBuffer()
    {
        if (buffer)
            glDeleteBuffers(1, &buffer);
    }

    void resizeBufferIfNecessary(size_t requestedSize, GLenum type)
    {
        if (requestedSize >= realSize)
        {
            while (requestedSize >= realSize)
                realSize = 2 * realSize + 1;
            GLuint newVBO;
            glGenBuffers(1, &newVBO);
            glBindBuffer(type, newVBO);
            if (CVS->supportsAsyncInstanceUpload())
            {
                glBufferStorage(type, realSize * sizeof(Data), 0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
                Pointer = static_cast<Data *>(glMapBufferRange(type, 0, realSize * sizeof(Data), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT));
            }
            else
                glBufferData(type, realSize * sizeof(Data), 0, GL_DYNAMIC_DRAW);

            if (buffer)
            {
                // Copy old data
                GLuint oldVBO = buffer;
                glBindBuffer(GL_COPY_WRITE_BUFFER, newVBO);
                glBindBuffer(GL_COPY_READ_BUFFER, oldVBO);
                glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, advertisedSize * sizeof(Data));
                glDeleteBuffers(1, &oldVBO);
            }
            buffer = newVBO;
        }
        advertisedSize = requestedSize;
    }

    Data *getPointer()
    {
        return Pointer;
    }

    size_t getSize() const
    {
        return advertisedSize;
    }

    GLuint getBuffer()
    {
        return buffer;
    }
};

template<typename Data>
class InstanceBuffer : public Singleton<InstanceBuffer<Data> >, public ArrayBuffer <Data>
{
public:
    InstanceBuffer() : ArrayBuffer<Data>()
    {
        ArrayBuffer<Data>::resizeBufferIfNecessary(10000, GL_ARRAY_BUFFER);
    }
};

template<typename VT>
struct VertexAttribBinder
{
public:
    static void bind();
};

template<typename VT>
struct InstanceAttribBinder
{
public:
    static void bind();
};




static void SetVertexAttrib_impl(size_t s)
{
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, s, 0);
    glVertexAttribDivisorARB(7, 1);
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, s, (GLvoid*)(3 * sizeof(float)));
    glVertexAttribDivisorARB(8, 1);
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, s, (GLvoid*)(6 * sizeof(float)));
    glVertexAttribDivisorARB(9, 1);
}



template<>
struct InstanceAttribBinder < InstanceDataShadow >
{
public:
    static void SetVertexAttrib()
    {
        SetVertexAttrib_impl(sizeof(InstanceDataShadow));
        glEnableVertexAttribArray(10);
        glVertexAttribIPointer(10, 2, GL_UNSIGNED_INT, sizeof(InstanceDataShadow), (GLvoid*)(9 * sizeof(float)));
        glVertexAttribDivisorARB(10, 1);
    }
};

template<>
struct InstanceAttribBinder<InstanceDataRSM>
{
public:
    static void SetVertexAttrib()
    {
        SetVertexAttrib_impl(sizeof(InstanceDataRSM));
        glEnableVertexAttribArray(10);
        glVertexAttribIPointer(10, 2, GL_UNSIGNED_INT, sizeof(InstanceDataRSM), (GLvoid*)(9 * sizeof(float)));
        glVertexAttribDivisorARB(10, 1);
    }
};

template<>
struct InstanceAttribBinder<InstanceDataDualTex>
{
public:
    static void SetVertexAttrib()
    {
        SetVertexAttrib_impl(sizeof(InstanceDataDualTex));
        glEnableVertexAttribArray(10);
        glVertexAttribIPointer(10, 2, GL_UNSIGNED_INT, sizeof(InstanceDataDualTex), (GLvoid*)(9 * sizeof(float)));
        glVertexAttribDivisorARB(10, 1);
        glEnableVertexAttribArray(11);
        glVertexAttribIPointer(11, 2, GL_UNSIGNED_INT, sizeof(InstanceDataDualTex), (GLvoid*)(9 * sizeof(float) + 2 * sizeof(unsigned)));
        glVertexAttribDivisorARB(11, 1);
    }
};

template<>
struct InstanceAttribBinder<InstanceDataThreeTex>
{
public:
    static void SetVertexAttrib()
    {
        SetVertexAttrib_impl(sizeof(InstanceDataThreeTex));
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
};

template<>
struct InstanceAttribBinder<GlowInstanceData>
{
public:
    static void SetVertexAttrib()
    {
        SetVertexAttrib_impl(sizeof(GlowInstanceData));
        glEnableVertexAttribArray(12);
        glVertexAttribPointer(12, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GlowInstanceData), (GLvoid*)(9 * sizeof(float)));
        glVertexAttribDivisorARB(12, 1);
    }
};


template<typename VT>
class VertexArrayObject : public Singleton<VertexArrayObject<VT> >
{
private:
    GLuint vao;
    std::unordered_map<irr::scene::IMeshBuffer*, std::pair<GLuint, GLuint> > mappedBaseVertexBaseIndex;

    void append(irr::scene::IMeshBuffer *mb)
    {
        size_t old_vtx_cnt = vbo.getSize();
        vbo.resizeBufferIfNecessary(old_vtx_cnt + mb->getVertexCount(), GL_ARRAY_BUFFER);
        size_t old_idx_cnt = ibo.getSize();
        ibo.resizeBufferIfNecessary(old_idx_cnt + mb->getIndexCount(), GL_ELEMENT_ARRAY_BUFFER);

        if (CVS->supportsAsyncInstanceUpload())
        {
            void *tmp = (char*)vbo.getPointer() + old_vtx_cnt * sizeof(VT);
            memcpy(tmp, mb->getVertices(), mb->getVertexCount() * sizeof(VT));
        }
        else
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbo.getBuffer());
            glBufferSubData(GL_ARRAY_BUFFER, old_vtx_cnt * sizeof(VT), mb->getVertexCount() * sizeof(VT), mb->getVertices());
        }
        if (CVS->supportsAsyncInstanceUpload())
        {
            void *tmp = (char*)ibo.getPointer() + old_idx_cnt * sizeof(irr::u16);
            memcpy(tmp, mb->getIndices(), mb->getIndexCount() * sizeof(irr::u16));
        }
        else
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo.getBuffer());
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, old_idx_cnt * sizeof(irr::u16), mb->getIndexCount() * sizeof(irr::u16), mb->getIndices());
        }

        mappedBaseVertexBaseIndex[mb] = std::make_pair(old_vtx_cnt, old_idx_cnt * sizeof(irr::u16));
    }

    void regenerateVAO()
    {
        glDeleteVertexArrays(1, &vao);
        vao = getNewVAO();
        glBindVertexArray(0);
    }

    void regenerateInstanceVao()
    {
        glDeleteVertexArrays(1, &vao_instanceDualTex);
        vao_instanceDualTex = VertexArrayObject<VT>::getInstance()->getNewVAO();
        glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer<InstanceDataDualTex>::getInstance()->getBuffer());
        InstanceAttribBinder<InstanceDataDualTex>::SetVertexAttrib();

        glDeleteVertexArrays(1, &vao_instanceThreeTex);
        vao_instanceThreeTex = VertexArrayObject<VT>::getInstance()->getNewVAO();
        glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer<InstanceDataThreeTex>::getInstance()->getBuffer());
        InstanceAttribBinder<InstanceDataThreeTex>::SetVertexAttrib();

        glDeleteVertexArrays(1, &vao_instanceShadow);
        vao_instanceShadow = VertexArrayObject<VT>::getInstance()->getNewVAO();
        glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer<InstanceDataShadow>::getInstance()->getBuffer());
        InstanceAttribBinder<InstanceDataShadow>::SetVertexAttrib();

        glDeleteVertexArrays(1, &vao_instanceRSM);
        vao_instanceRSM = VertexArrayObject<VT>::getInstance()->getNewVAO();
        glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer<InstanceDataRSM>::getInstance()->getBuffer());
        InstanceAttribBinder<InstanceDataRSM>::SetVertexAttrib();

        glDeleteVertexArrays(1, &vao_instanceGlowData);
        vao_instanceGlowData = VertexArrayObject<VT>::getInstance()->getNewVAO();
        glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer<GlowInstanceData>::getInstance()->getBuffer());
        InstanceAttribBinder<GlowInstanceData>::SetVertexAttrib();

        glBindVertexArray(0);
    }
public:
    GLuint vao_instanceDualTex, vao_instanceThreeTex, vao_instanceShadow, vao_instanceRSM, vao_instanceGlowData;

    ArrayBuffer<VT> vbo;
    ArrayBuffer<irr::u16> ibo;

    VertexArrayObject()
    {
        glGenVertexArrays(1, &vao);
        glGenVertexArrays(1, &vao_instanceDualTex);
        glGenVertexArrays(1, &vao_instanceThreeTex);
        glGenVertexArrays(1, &vao_instanceShadow);
        glGenVertexArrays(1, &vao_instanceRSM);
        glGenVertexArrays(1, &vao_instanceGlowData);
    }

    ~VertexArrayObject()
    {
        glDeleteVertexArrays(1, &vao);
        glDeleteVertexArrays(1, &vao_instanceDualTex);
        glDeleteVertexArrays(1, &vao_instanceThreeTex);
        glDeleteVertexArrays(1, &vao_instanceShadow);
        glDeleteVertexArrays(1, &vao_instanceRSM);
        glDeleteVertexArrays(1, &vao_instanceGlowData);
    }

    GLuint getNewVAO()
    {
        GLuint newvao;
        glGenVertexArrays(1, &newvao);
        glBindVertexArray(newvao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.getBuffer());
        VertexAttribBinder<VT>::bind();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo.getBuffer());
        return newvao;
    }

    std::pair<unsigned, unsigned> getBase(irr::scene::IMeshBuffer *mb)
    {
        if (mappedBaseVertexBaseIndex.find(mb) == mappedBaseVertexBaseIndex.end())
        {
            append(mb);
            regenerateVAO();
            regenerateInstanceVao();
        }
        return mappedBaseVertexBaseIndex[mb];
    }

    bool isEmpty() const
    {
        return ibo.getSize() == 0;
    }

    GLuint getVAO()
    {
        return vao;
    }
};

class VAOManager : public Singleton<VAOManager>
{
    enum VTXTYPE { VTXTYPE_STANDARD, VTXTYPE_TCOORD, VTXTYPE_TANGENT, VTXTYPE_COUNT };
    irr::video::E_VERTEX_TYPE getVertexType(enum VTXTYPE tp);
public:
    VAOManager();
    std::pair<unsigned, unsigned> getBase(irr::scene::IMeshBuffer *, void * = nullptr);
    GLuint getInstanceBuffer(InstanceType it);
    void *getInstanceBufferPtr(InstanceType it);
    unsigned getVAO(irr::video::E_VERTEX_TYPE type, bool skinned);
    unsigned getInstanceVAO(irr::video::E_VERTEX_TYPE vt, bool skinned, enum InstanceType it);
    ~VAOManager();
};

#endif