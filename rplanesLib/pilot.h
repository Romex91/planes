#pragma once
#include "stdafx.h"
namespace rplanes
{
	namespace playerdata
	{
#define PILOT_SKILL(x) \
	private:\
	size_t x##_; \
	public: \
	float get_skill_##x(bool isDefected);\
	void up_##x(size_t exp){ if (exp_ > exp){ x##_ += exp; exp_ -= exp; } }\

#pragma db value
		class Pilot
		{
		private:
			size_t exp_;
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
			PILOT_SKILL(engine);
			PILOT_SKILL(flight);
			PILOT_SKILL(endurance);
			PILOT_SKILL(shooting);

			float getExp();
			void addExp(int exp);
		};
#undef PILOT_SKILL
	}
}
