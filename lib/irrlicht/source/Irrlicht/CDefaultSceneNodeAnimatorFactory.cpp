// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CDefaultSceneNodeAnimatorFactory.h"
#include "ICursorControl.h"
#include "ISceneNodeAnimatorCollisionResponse.h"
#include "ISceneManager.h"

namespace irr
{
namespace scene
{

//! Names for scene node types
const c8* const SceneNodeAnimatorTypeNames[] =
{
	"flyCircle",
	"flyStraight",
	"followSpline",
	"rotation",
	"texture",
	"deletion",
	"collisionResponse",
	"cameraFPS",
	"cameraMaya",
	0
};


CDefaultSceneNodeAnimatorFactory::CDefaultSceneNodeAnimatorFactory(ISceneManager* mgr, gui::ICursorControl* crs)
: Manager(mgr), CursorControl(crs)
{
	#ifdef _DEBUG
	setDebugName("CDefaultSceneNodeAnimatorFactory");
	#endif

	// don't grab the scene manager here to prevent cyclic references
	if (CursorControl)
		CursorControl->grab();
}


CDefaultSceneNodeAnimatorFactory::~CDefaultSceneNodeAnimatorFactory()
{
	if (CursorControl)
		CursorControl->drop();
}


//! creates a scene node animator based on its type id
ISceneNodeAnimator* CDefaultSceneNodeAnimatorFactory::createSceneNodeAnimator(ESCENE_NODE_ANIMATOR_TYPE type, ISceneNode* target)
{
	scene::ISceneNodeAnimator* anim = 0;

	switch(type)
	{
	default:
		break;
	}

	if (anim && target)
		target->addAnimator(anim);

	return anim;
}


//! creates a scene node animator based on its type name
ISceneNodeAnimator* CDefaultSceneNodeAnimatorFactory::createSceneNodeAnimator(const c8* typeName, ISceneNode* target)
{
	return createSceneNodeAnimator( getTypeFromName(typeName), target );
}


//! returns amount of scene node animator types this factory is able to create
u32 CDefaultSceneNodeAnimatorFactory::getCreatableSceneNodeAnimatorTypeCount() const
{
	return ESNAT_COUNT;
}


//! returns type of a createable scene node animator type
ESCENE_NODE_ANIMATOR_TYPE CDefaultSceneNodeAnimatorFactory::getCreateableSceneNodeAnimatorType(u32 idx) const
{
	if (idx<ESNAT_COUNT)
		return (ESCENE_NODE_ANIMATOR_TYPE)idx;
	else
		return ESNAT_UNKNOWN;
}


//! returns type name of a createable scene node animator type 
const c8* CDefaultSceneNodeAnimatorFactory::getCreateableSceneNodeAnimatorTypeName(u32 idx) const
{
	if (idx<ESNAT_COUNT)
		return SceneNodeAnimatorTypeNames[idx];
	else
		return 0;
}

//! returns type name of a createable scene node animator type 
const c8* CDefaultSceneNodeAnimatorFactory::getCreateableSceneNodeAnimatorTypeName(ESCENE_NODE_ANIMATOR_TYPE type) const
{
	// for this factory: index == type

	if (type<ESNAT_COUNT)
		return SceneNodeAnimatorTypeNames[type];
	else
		return 0;
}

ESCENE_NODE_ANIMATOR_TYPE CDefaultSceneNodeAnimatorFactory::getTypeFromName(const c8* name) const
{
	for ( u32 i=0; SceneNodeAnimatorTypeNames[i]; ++i)
		if (!strcmp(name, SceneNodeAnimatorTypeNames[i]) )
			return (ESCENE_NODE_ANIMATOR_TYPE)i;

	return ESNAT_UNKNOWN;
}


} // end namespace scene
} // end namespace irr

