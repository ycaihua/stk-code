// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#include "CParticleSphereEmitter.h"
#include "../../os.h"
#include "IAttributes.h"

namespace irr
{
namespace scene
{

//! constructor
CParticleSphereEmitter::CParticleSphereEmitter(
	const core::vector3df& center, f32 radius,
	const core::vector3df& direction, u32 minParticlesPerSecond,
	u32 maxParticlesPerSecond, const video::SColor& minStartColor,
	const video::SColor& maxStartColor, u32 lifeTimeMin, u32 lifeTimeMax,
	s32 maxAngleDegrees,
	const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize )
	: Center(center), Radius(radius), Direction(direction),
	MinStartSize(minStartSize), MaxStartSize(maxStartSize),
	MinParticlesPerSecond(minParticlesPerSecond),
	MaxParticlesPerSecond(maxParticlesPerSecond),
	MinStartColor(minStartColor), MaxStartColor(maxStartColor),
	MinLifeTime(lifeTimeMin), MaxLifeTime(lifeTimeMax),
	Time(0), Emitted(0), MaxAngleDegrees(maxAngleDegrees)
{
	#ifdef _DEBUG
	setDebugName("CParticleSphereEmitter");
	#endif

}


//! Prepares an array with new particles to emitt into the system
//! and returns how much new particles there are.
s32 CParticleSphereEmitter::emitt(u32 now, u32 timeSinceLastCall, SParticle*& outArray)
{
	Time += timeSinceLastCall;

	const u32 pps = (MaxParticlesPerSecond - MinParticlesPerSecond);
	const f32 perSecond = pps ? ((f32)MinParticlesPerSecond + os::Randomizer::frand() * pps) : MinParticlesPerSecond;
	const f32 everyWhatMillisecond = 1000.0f / perSecond;

	if(Time > everyWhatMillisecond)
	{
		Particles.set_used(0);
		u32 amount = (u32)((Time / everyWhatMillisecond) + 0.5f);
		Time = 0;
		SParticle p;

		if(amount > MaxParticlesPerSecond*2)
			amount = MaxParticlesPerSecond * 2;

		for(u32 i=0; i<amount; ++i)
		{
			// Random distance from center
			const f32 distance = os::Randomizer::frand() * Radius;

			// Random direction from center
			p.pos.set(Center + distance);
			p.pos.rotateXYBy(os::Randomizer::frand() * 360.f, Center );
			p.pos.rotateYZBy(os::Randomizer::frand() * 360.f, Center );
			p.pos.rotateXZBy(os::Randomizer::frand() * 360.f, Center );

			p.startTime = now;
			p.vector = Direction;

			if(MaxAngleDegrees)
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

