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
struct InstanceDataSingleTex
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

template<typename VT>
class VertexAttribBinder
{
public:
    static void bind();
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
            void *tmp = (char*)ibo.getPointer() + old_idx_cnt * sizeof(u16);
            memcpy(tmp, mb->getIndices(), mb->getIndexCount() * sizeof(u16));
        }
        else
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo.getBuffer());
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, old_idx_cnt * sizeof(u16), mb->getIndexCount() * sizeof(u16), mb->getIndices());
        }

        mappedBaseVertexBaseIndex[mb] = std::make_pair(old_vtx_cnt, old_idx_cnt * sizeof(u16));
    }

    void regenerateVAO()
    {
        glDeleteVertexArrays(1, &vao);
        vao = getNewVAO();
        glBindVertexArray(0);
    }
public:
    bool dirty;

    ArrayBuffer<VT> vbo;
    ArrayBuffer<irr::u16> ibo;

    VertexArrayObject() : dirty(true)
    {
        glGenVertexArrays(1, &vao);
    }

    ~VertexArrayObject()
    {
        glDeleteVertexArrays(1, &vao);
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
            dirty = true;
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
    GLuint vbo[VTXTYPE_COUNT], ibo[VTXTYPE_COUNT], vao[VTXTYPE_COUNT];
    GLuint instance_vbo[InstanceTypeCount];
    void *Ptr[InstanceTypeCount];
    void *VBOPtr[VTXTYPE_COUNT], *IBOPtr[VTXTYPE_COUNT];
    size_t RealVBOSize[VTXTYPE_COUNT], RealIBOSize[VTXTYPE_COUNT];
    size_t last_vertex[VTXTYPE_COUNT], last_index[VTXTYPE_COUNT];
    std::unordered_map<irr::scene::IMeshBuffer*, unsigned> mappedBaseVertex[VTXTYPE_COUNT], mappedBaseIndex[VTXTYPE_COUNT];
    std::map<std::pair<irr::video::E_VERTEX_TYPE, InstanceType>, GLuint> InstanceVAO;

    void cleanInstanceVAOs();
    void regenerateInstancedVAO();
    size_t getVertexPitch(enum VTXTYPE) const;
    VTXTYPE getVTXTYPE(irr::video::E_VERTEX_TYPE type);
    irr::video::E_VERTEX_TYPE getVertexType(enum VTXTYPE tp);
public:
    VAOManager();
    std::pair<unsigned, unsigned> getBase(irr::scene::IMeshBuffer *);
    GLuint getInstanceBuffer(InstanceType it);
    void *getInstanceBufferPtr(InstanceType it);
    unsigned getVBO(irr::video::E_VERTEX_TYPE type);
    void *getVBOPtr(irr::video::E_VERTEX_TYPE type);
    unsigned getVAO(irr::video::E_VERTEX_TYPE type);
    unsigned getInstanceVAO(irr::video::E_VERTEX_TYPE vt, enum InstanceType it);
    ~VAOManager();
};

#endif