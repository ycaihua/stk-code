// Copyright (C) 2010-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef C_PARTICLE_SCALE_AFFECTOR_H
#define C_PARTICLE_SCALE_AFFECTOR_H

#include <IParticleAffector.h>

namespace irr
{
	namespace scene
	{
		class CParticleScaleAffector : public IParticleAffector
		{
		public:
			CParticleScaleAffector(const core::dimension2df& scaleTo = core::dimension2df(1.0f, 1.0f));

			virtual void affect(u32 now, SParticle *particlearray, u32 count);

			//! Get emitter type
			virtual E_PARTICLE_AFFECTOR_TYPE getType() const;

		protected:
			core::dimension2df ScaleTo;
		};
	}
}


#endif // C_PARTICLE_SCALE_AFFECTOR_H

