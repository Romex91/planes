#pragma once
#include "stdafx.h"
#include "player.h"


class BotTargetsStorage
{
public:
	void selectTarget(size_t botId, size_t targetId);
	void deselect( size_t botId );
	size_t countAttackers(size_t botId);
	//returns botId if the bot has no targets 
	size_t getTarget(size_t botId);
	void clear();
private:
	//target list for each boy
	std::map< size_t, size_t > targets;
	//key - bot id, value - attackers count
	std::map< size_t, size_t> nAttackersMap;
};

class BotCondition
{
public:
	void control(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage)
	{
		control_derv(player,frameTime, botTargetsStorage);
		player.setControllable(controllable_);
	}

	virtual void initializeConditionList() = 0;

	std::shared_ptr<BotCondition> handleConditionList(Player & player);
protected:
	virtual void control_derv(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage) = 0;

	//exp determines how aggressive would be acceleration for engine limits [~0.5, 1]
	void accelerate(Player & player,
		unsigned short speed,
		float exp,
		short increaseValue,
		short decreaseValue );
	void turnToPoint(Player & player,
		rplanes::PointXY point,
		//[0,100]
		unsigned short maxFaint,
		//turn intensivity 
		unsigned short turnValue,
		float angleExp
		);
	rplanes::serverdata::Plane::ControllableParameters controllable_;
	std::vector< std::pair < std::function< bool( Player & ) >, std::shared_ptr<BotCondition> > > conditionList_;
};

namespace botconditions
{
	namespace easy
	{
		class Patrol : public BotCondition
		{
		public:
			virtual void initializeConditionList();
		protected:
			virtual void control_derv(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage);

		private:
			float watchAngle_;
		};

		class Flee : public BotCondition
		{
		public:
			virtual void initializeConditionList();
		protected:
			virtual void control_derv(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage);
		protected:
			float fleeTimer_;
			short fleeTurnVal_;
		};

		class Attack : public BotCondition
		{
		public:
			virtual void initializeConditionList();
		protected:
			virtual void control_derv(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage);
		private:
			void selectTarget(Player & player, std::vector< DestroyablePlane * > & targets, BotTargetsStorage & botTargetsStorage);
			// < 0 means no target found
			int targetNo_;
			float selectTimer_;
		};

		class Escape : public BotCondition
		{
		public:
			virtual void initializeConditionList();
		protected:
			virtual void control_derv(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage);
		private:
			float panicTime_;
		};
	}
	class Peacefull : public BotCondition
	{
	public:
		virtual void initializeConditionList();
		Peacefull();
	protected:
		virtual void control_derv(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage);
	private:
		float hitCoolDown;
		unsigned short hitTurn;
	};
}

class Bot
{
public:
	enum Type
	{
		EASY,
		HARD,
		PEACEFULL
	};
	void control(float frameTime, BotTargetsStorage & botTargetsStorage);
	Bot(std::shared_ptr<Player > player, Type type);
	std::shared_ptr< Player > getPlayer();

private:
	std::shared_ptr< BotCondition > condition_;
	std::shared_ptr< Player > player_;
};
