#include "pilot.h"
#include "configuration.h"
#include <math.h>

namespace rplanes
{
	namespace playerdata
	{
#define PILOT_GETTER(x) float Pilot::get_skill_##x( bool isDefected )\
		{ \
		float retval = log10( x##_ )/2.f; \
		if(isDefected)\
		return retval * configuration().defect.pilotSkillsFactor;\
		return retval;\
		}

		PILOT_GETTER(engine);
		PILOT_GETTER(flight);
		PILOT_GETTER(endurance);
		PILOT_GETTER(shooting);
#undef PILOT_GETTER


		Pilot::Pilot()
		{
			engine_ = 10000;
			flight_ = 10000;
			endurance_ = 10000;
			shooting_ = 10000;
			exp_ = configuration().profile.startExp;
		}

		float Pilot::getExp()
		{
			return exp_;
		}

		void Pilot::addExp( int exp )
		{
			exp_+= exp;
		}
	}
}
