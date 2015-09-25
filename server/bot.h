#pragma once
#include "stdafx.h"
#include "player.h"


class BotTargetsStorage
{
public:
	void selectTarget(size_t botId, size_t targetId);
	void deselect( size_t botId );
	size_t countAttackers(size_t botId);
	//если бот не имеет цели, вернет botId
	size_t getTarget(size_t botId);
	void clear();
private:
	//список целей каждого бота
	std::map< size_t, size_t > targets;
	//количество атакующих ботов
	std::map< size_t, size_t> nAttackersMap;
};

class BotCondition
{
public:
	//управление ботом
	void control(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage)
	{
		control_derv(player,frameTime, botTargetsStorage);
		player.setControllable(controllable_);
	}
	//инициализация списка переходных состояний
	virtual void initializeConditionList() = 0;
	//обработка списка переходных состояний
	std::shared_ptr<BotCondition> handleConditionList(Player & player);
protected:
	virtual void control_derv(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage) = 0;
	//угол берется относительно носа самолета

	void accelerate(Player & player,
		//требуемая скорость
		unsigned short speed,
		//точность следования критическим показателям. Чем ближе к еденице тем быстрее разгон при значениях > 1 двигатель будет повреждаться
		float exp,
		//скорость набора мощности
		short increaseValue,
		//скорость сброса мощности
		short decreaseValue );
	void turnToPoint(Player & player,
		rplanes::PointXY point,
		//уровень обморока до которого будет происходить набор угловой скорости. от 0 до 100
		unsigned short maxFaint,
		//интенсивность поворота
		unsigned short turnValue,
		//точность следования направлению
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
			//если < 0 значит подходящая цель не найдена
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
