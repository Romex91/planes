#include "roomMessages.h"
#define _USE_MATH_DEFINES
#include < math.h >
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>


void rplanes::network::MCreatePlanes::Plane::extrapolate(float frameTime)
{
	//Translational motion
	extrapolationData.speed += extrapolationData.acceleration * frameTime;
	pos.x += extrapolationData.speed * std::cos(pos.angle / 180.0 * M_PI) * frameTime * configuration().flight.speedFactor;
	pos.y += extrapolationData.speed * std::sin(pos.angle / 180.0 * M_PI) * frameTime * configuration().flight.speedFactor;
	//rotation
	pos.angle += extrapolationData.angleVelocity
		* frameTime * configuration().flight.angleVelocityFactor;
}

void rplanes::network::MCreatePlanes::Plane::init(const serverdata::Plane & Plane, std::string PlayerName, size_t ID)
{
	planeName = Plane.name;
	playerName = PlayerName;
	id = ID;
	nation = Plane.nation;
	pos.x = Plane.position.x;
	pos.y = Plane.position.y;
	pos.roll = Plane.position.roll;
	pos.angle = Plane.position.angle;

	extrapolationData.acceleration = Plane.target.acceleration;
	extrapolationData.angleVelocity = Plane.target.angularVelocity;
	extrapolationData.speed = Plane.target.V;
	modules.clear();
	for (auto & module : Plane.modules)
	{
		Module m;
		m.defected = module->hp.isDefected();
		m.hp = module->hp;
		m.hpMax = module->hpMax;
		m.hitZone = module->hitZone;
		m.type = module->getType();
		modules.push_back(m);
	}
}

void rplanes::network::MRoomInfo::clear()
{
	newPlayers.clear();
	disconnectedPlayers.clear();
	updatedStatistics.clear();
}

void rplanes::network::MInterfaceData::update(const serverdata::Plane & Plane)
{
	faintVal = Plane.target.faintVal * 100;
	aimSize = Plane.target.aimSize;
	shootingDistance = Plane.target.clientShootingDistance;
	Vmax = Plane.statical.Vmax;
	Vmin = Plane.statical.Vmin;
	V = Plane.target.V;

	ammunitions.clear();
	for (auto &serverAmmo : Plane.ammunitions)
	{
		Ammo ammo;
		ammo.caliber = static_cast<unsigned short>(serverAmmo.caliber);
		ammo.capacity = static_cast<unsigned short>(serverAmmo.capacity);
		ammunitions.push_back(ammo);
	}
	thermometers.clear();
	for (auto & engine : Plane.engines)
	{
		Thermometer newTherm;
		newTherm.criticalTemperature = engine.criticalTemperature * 10;
		newTherm.dT = engine.dt * 10;
		newTherm.dTmax = engine.maxDt * 10;
		newTherm.temperature = engine.temperature * 10;
		thermometers.push_back(newTherm);
	}
	gasTank.fuel = 0;
	gasTank.capacity = 0;
	for (auto & Tank : Plane.tanks)
	{
		gasTank.fuel += Tank.fuel;
		gasTank.capacity += Tank.capacity;
	}
}
