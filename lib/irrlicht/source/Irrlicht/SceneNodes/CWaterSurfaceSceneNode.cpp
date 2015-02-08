// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CWaterSurfaceSceneNode.h"
#include <Core/ISceneManager.h>
#include <Util/IMeshManipulator.h>
#include "IMeshCache.h"
#include <Core/S3DVertex.h>
#include "SMesh.h"
#include "../os.h"

namespace irr
{
namespace scene
{

//! constructor
CWaterSurfaceSceneNode::CWaterSurfaceSceneNode(f32 waveHeight, f32 waveSpeed, f32 waveLength,
		IMesh* mesh, ISceneNode* parent, ISceneManager* mgr, s32 id,
		const core::vector3df& position, const core::vector3df& rotation,
		const core::vector3df& scale)
	: CMeshSceneNode(mesh, parent, mgr, id, position, rotation, scale),
	WaveLength(waveLength), WaveSpeed(waveSpeed), WaveHeight(waveHeight),
	OriginalMesh(0)
{
	#ifdef _DEBUG
	setDebugName("CWaterSurfaceSceneNode");
	#endif

	setMesh(mesh);
}


//! destructor
CWaterSurfaceSceneNode::~CWaterSurfaceSceneNode()
{
	// Mesh is dropped in CMeshSceneNode destructor
	if (OriginalMesh)
		OriginalMesh->drop();
}


//! frame
void CWaterSurfaceSceneNode::OnRegisterSceneNode()
{
	CMeshSceneNode::OnRegisterSceneNode();
}


void CWaterSurfaceSceneNode::OnAnimate(u32 timeMs)
{
	if (Mesh && IsVisible)
	{
		const u32 meshBufferCount = Mesh->getMeshBufferCount();
		const f32 time = timeMs / WaveSpeed;

		for (u32 b=0; b<meshBufferCount; ++b)
		{
			const u32 vtxCnt = Mesh->getMeshBuffer(b)->getVertexCount();

			for (u32 i=0; i<vtxCnt; ++i)
				Mesh->getMeshBuffer(b)->getPosition(i).Y = addWave(
					OriginalMesh->getMeshBuffer(b)->getPosition(i),
					time);
		}// end for all mesh buffers
		Mesh->setDirty(scene::EBT_VERTEX);

		SceneManager->getMeshManipulator()->recalculateNormals(Mesh);
	}
	CMeshSceneNode::OnAnimate(timeMs);
}


void CWaterSurfaceSceneNode::setMesh(IMesh* mesh)
{
	CMeshSceneNode::setMesh(mesh);
	if (!mesh)
		return;
	if (OriginalMesh)
		OriginalMesh->drop();
	IMesh* clone = SceneManager->getMeshManipulator()->createMeshCopy(mesh);
	OriginalMesh = mesh;
	Mesh = clone;
	Mesh->setHardwareMappingHint(scene::EHM_STATIC, scene::EBT_INDEX);
//	Mesh->setHardwareMappingHint(scene::EHM_STREAM, scene::EBT_VERTEX);
}

f32 CWaterSurfaceSceneNode::addWave(const core::vector3df &source, f32 time) const
{
	return source.Y +
		(sinf(((source.X/WaveLength) + time)) * WaveHeight) +
		(cosf(((source.Z/WaveLength) + time)) * WaveHeight);
}

} // end namespace scene
} // end namespace irr

