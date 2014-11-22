#pragma once
#include "stdafx.h"
namespace rplanes
{
	namespace playerdata
	{
#define PILOT_GETTER(x) float get_skill_##x( bool isDefected );
#define PILOT_LEVEL_UP(x) void up_##x( size_t exp ){ if(exp_ > exp){ x##_ += exp; exp_ -= exp; } }
#pragma db value
		class Pilot
		{
		private:
			size_t engine_,	//навыки : двигатель,
				flight_,		//маневренность,
				endurance_,	//выносливость,
				shooting_,	// стрельба.
				exp_;
			friend class odb::access;
		public:
			template<class Archive>
			void serialize(Archive & ar, const unsigned int version)
			{
				ar & engine_;
				ar & flight_;
				ar & endurance_;
				ar & shooting_;
				ar & exp_;
			}


			Pilot();
			PILOT_GETTER(engine);
			PILOT_GETTER(flight);
			PILOT_GETTER(endurance);
			PILOT_GETTER(shooting);

			PILOT_LEVEL_UP(engine);
			PILOT_LEVEL_UP(flight);
			PILOT_LEVEL_UP(endurance);
			PILOT_LEVEL_UP(shooting);
			float getExp();
			void addExp(int exp);
		};
#undef PILOT_GETTER
#undef PILOT_LEVEL_UP
	}
}
