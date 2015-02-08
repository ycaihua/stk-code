// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CParticleBoxEmitter.h"
#include "../../os.h"
#include "IAttributes.h"
#include "irrMath.h"

namespace irr
{
namespace scene
{

//! constructor
CParticleBoxEmitter::CParticleBoxEmitter(
	const core::aabbox3df& box, const core::vector3df& direction,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	video::SColor minStartColor, video::SColor maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax, s32 maxAngleDegrees,
	const core::dimension2df& minStartSize, const core::dimension2df& maxStartSize)
 : Box(box), Direction(direction),
	MaxStartSize(maxStartSize), MinStartSize(minStartSize),
	MinParticlesPerSecond(minParticlesPerSecond),
	MaxParticlesPerSecond(maxParticlesPerSecond),
	MinStartColor(minStartColor), MaxStartColor(maxStartColor),
	MinLifeTime(lifeTimeMin), MaxLifeTime(lifeTimeMax),
	Time(0), Emitted(0), MaxAngleDegrees(maxAngleDegrees)
{
	#ifdef _DEBUG
	setDebugName("CParticleBoxEmitter");
	#endif
}


//! Prepares an array with new particles to emitt into the system
//! and returns how much new particles there are.
s32 CParticleBoxEmitter::emitt(u32 now, u32 timeSinceLastCall, SParticle*& outArray)
{
	Time += timeSinceLastCall;

	const u32 pps = (MaxParticlesPerSecond - MinParticlesPerSecond);
	const f32 perSecond = pps ? ((f32)MinParticlesPerSecond + os::Randomizer::frand() * pps) : MinParticlesPerSecond;
	const f32 everyWhatMillisecond = 1000.0f / perSecond;

	if (Time > everyWhatMillisecond)
	{
		Particles.set_used(0);
		u32 amount = (u32)((Time / everyWhatMillisecond) + 0.5f);
		Time = 0;
		SParticle p;
		const core::vector3df& extent = Box.getExtent();

		if (amount > MaxParticlesPerSecond*2)
			amount = MaxParticlesPerSecond * 2;

		for (u32 i=0; i<amount; ++i)
		{
			p.pos.X = Box.MinEdge.X + os::Randomizer::frand() * extent.X;
			p.pos.Y = Box.MinEdge.Y + os::Randomizer::frand() * extent.Y;
			p.pos.Z = Box.MinEdge.Z + os::Randomizer::frand() * extent.Z;

			p.startTime = now;
			p.vector = Direction;

			if (MaxAngleDegrees)
			{
				core::vector3df tgt = Direction;
				tgt.rotateXYBy(os::Randomizer::frand() * MaxAngleDegrees);
				tgt.rotateYZBy(os::Randomizer::frand() * MaxAngleDegrees);
				tgt.rotateXZBy(os::Randomizer::frand() * MaxAngleDegrees);
				p.vector = tgt;
			}

			p.endTime = now + MinLifeTime;
			if (MaxLifeTime != MinLifeTime)
				p.endTime += os::Randomizer::rand() % (MaxLifeTime - MinLifeTime);

			if (MinStartColor==MaxStartColor)
				p.color=MinStartColor;
			else
				p.color = MinStartColor.getInterpolated(MaxStartColor, os::Randomizer::frand());

			p.startColor = p.color;
			p.startVector = p.vector;

			if (MinStartSize==MaxStartSize)
				p.startSize = MinStartSize;
			else
				p.startSize = MinStartSize.getInterpolated(MaxStartSize, os::Randomizer::frand());
			p.size = p.startSize;

			Particles.push_back(p);
		}

		outArray = Particles.pointer();

		return Particles.size();
	}

	return 0;
}

} // end namespace scene
} // end namespace irr

