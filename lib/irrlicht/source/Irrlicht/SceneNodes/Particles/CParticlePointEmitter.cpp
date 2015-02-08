// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CParticlePointEmitter.h"
#include "../../os.h"
#include "IAttributes.h"

namespace irr
{
namespace scene
{

//! constructor
CParticlePointEmitter::CParticlePointEmitter(
	const core::vector3df& direction, u32 minParticlesPerSecond,
	u32 maxParticlesPerSecond, video::SColor minStartColor,
	video::SColor maxStartColor, u32 lifeTimeMin, u32 lifeTimeMax,
	s32 maxAngleDegrees,
	const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize)
 : Direction(direction),
	MinStartSize(minStartSize), MaxStartSize(maxStartSize),
	MinParticlesPerSecond(minParticlesPerSecond),
	MaxParticlesPerSecond(maxParticlesPerSecond),
	MinStartColor(minStartColor), MaxStartColor(maxStartColor),
	MinLifeTime(lifeTimeMin), MaxLifeTime(lifeTimeMax),
	MaxAngleDegrees(maxAngleDegrees), Time(0), Emitted(0)
{
	#ifdef _DEBUG
	setDebugName("CParticlePointEmitter");
	#endif
}


//! Prepares an array with new particles to emitt into the system
//! and returns how much new particles there are.
s32 CParticlePointEmitter::emitt(u32 now, u32 timeSinceLastCall, SParticle*& outArray)
{
	Time += timeSinceLastCall;

	const u32 pps = (MaxParticlesPerSecond - MinParticlesPerSecond);
	const f32 perSecond = pps ? ((f32)MinParticlesPerSecond + os::Randomizer::frand() * pps) : MinParticlesPerSecond;
	const f32 everyWhatMillisecond = 1000.0f / perSecond;

	if (Time > everyWhatMillisecond)
	{
		Time = 0;
		Particle.startTime = now;
		Particle.vector = Direction;

		if (MaxAngleDegrees)
		{
			core::vector3df tgt = Direction;
			tgt.rotateXYBy(os::Randomizer::frand() * MaxAngleDegrees);
			tgt.rotateYZBy(os::Randomizer::frand() * MaxAngleDegrees);
			tgt.rotateXZBy(os::Randomizer::frand() * MaxAngleDegrees);
			Particle.vector = tgt;
		}

		Particle.endTime = now + MinLifeTime;
		if (MaxLifeTime != MinLifeTime)
			Particle.endTime += os::Randomizer::rand() % (MaxLifeTime - MinLifeTime);

		if (MinStartColor==MaxStartColor)
			Particle.color=MinStartColor;
		else
			Particle.color = MinStartColor.getInterpolated(MaxStartColor, os::Randomizer::frand());

		Particle.startColor = Particle.color;
		Particle.startVector = Particle.vector;

		if (MinStartSize==MaxStartSize)
			Particle.startSize = MinStartSize;
		else
			Particle.startSize = MinStartSize.getInterpolated(MaxStartSize, os::Randomizer::frand());
		Particle.size = Particle.startSize;

		outArray = &Particle;
		return 1;
	}

	return 0;
}


} // end namespace scene
} // end namespace irr

