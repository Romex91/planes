#pragma once
#include "stdafx.h"
#include "player.h"


class BotTargetsStorage
{
public:
	void selectTarget(size_t botId, size_t targetId)
	{
		size_t comparableValue;
		auto compare = [&comparableValue](const std::pair<size_t, size_t> & i)->bool
		{
			return i.second == comparableValue && i.second != i.first;
		};


		//���� ��� ���� ����, ������������� ���������� �������, ��������� � ���
		if ( targets.count(botId) != 0 )
		{
			comparableValue = targets[botId];
			nAttackersMap[targets[botId]] = count_if(targets.begin(), targets.end(), compare);
		}

		//����������� ����� ����
		targets[botId] = targetId;

		//������������� ���������� �������, ��������� � ����� ����
		comparableValue = targetId;
		nAttackersMap[targetId] = count_if(targets.begin(), targets.end(), compare);
	}
	void deselect( size_t botId )
	{
		selectTarget(botId, botId);
	}
	size_t countAttackers(size_t botId)
	{
		auto i = nAttackersMap.find(botId);
		if ( i != nAttackersMap.end() )
		{
			return i->second;
		}
		return 0;
	}
	//���� ��� �� ����� ����, ������ botId
	size_t getTarget(size_t botId)
	{
		auto i = targets.find(botId);
		if ( i != targets.end() )
		{
			return i->second;
		}
		return botId;
	}
	void clear()
	{
		targets.clear();
		nAttackersMap.clear();
	}
private:
	//������ ����� ������� ����
	std::map< size_t, size_t > targets;
	//���������� ��������� �����
	std::map< size_t, size_t> nAttackersMap;
};

class BotCondition
{
public:
	//���������� �����
	void control(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage)
	{
		control_derv(player,frameTime, botTargetsStorage);
		player.setControllable(controllable_);
	}
	//������������� ������ ���������� ���������
	virtual void initializeConditionList() = 0;
	//��������� ������ ���������� ���������
	std::shared_ptr<BotCondition> handleConditionList(Player & player);
protected:
	virtual void control_derv(Player & player, float frameTime, BotTargetsStorage & botTargetsStorage) = 0;
	//���� ������� ������������ ���� ��������

	void accelerate(Player & player,
		//��������� ��������
		unsigned short speed,
		//�������� ���������� ����������� �����������. ��� ����� � ������� ��� ������� ������ ��� ��������� > 1 ��������� ����� ������������
		float exp,
		//�������� ������ ��������
		short increaseValue,
		//�������� ������ ��������
		short decreaseValue );
	void turnToPoint(Player & player,
		rplanes::PointXY point,
		//������� �������� �� �������� ����� ����������� ����� ������� ��������. �� 0 �� 100
		unsigned short maxFaint,
		//������������� ��������
		unsigned short turnValue,
		//�������� ���������� �����������
		float angleExp
		);
	
	//std::vector< Player::DestroyablePlane * > getEnemies(Player & player, float radius = -1.f);
	//std::vector< Player::DestroyablePlane * > getFriends(Player & player, float radius = -1.f);
	//std::vector<Player::DestroyablePlane *> getPlanesInSector(Player & player, std::vector<Player::DestroyablePlane *> planes, float angle, float sector)
	//{
	//	std::vector<Player::DestroyablePlane *> retval;

	//	for (auto & plane : planes)
	//	{
	//		float innerAngle = (rplanes::angleFromPoints(player.getPosition(), plane->position)
	//			- rplanes::angleFromPoints(player.getPrevPosition(), player.getPosition()));
	//		while (innerAngle < 0.f)
	//		{
	//			innerAngle += 360.f;
	//		}
	//		while (angle < 0.f)
	//		{
	//			angle += 360.f;
	//		}
	//		while (angle > 360.f)
	//		{
	//			angle -= 360.f;
	//		}
	//		float angleResidual = std::abs(innerAngle - angle);
	//		if (angleResidual > 180.f)
	//			angleResidual = 360.f - angleResidual;

	//		if (angleResidual < sector / 2)
	//		{
	//			retval.push_back(plane);
	//		}
	//	}
	//	return retval;
	//}
	//Player::DestroyablePlane & getThisPlane(Player & player);

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
			//���� < 0 ������ ���������� ���� �� �������
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
	void control(float frameTime, BotTargetsStorage & botTargetsStorage)
	{
		if ( !condition_ || !player_ )
		{
			throw rplanes::eRoomError("������ ����. ");
		}
		condition_->control(*player_, frameTime, botTargetsStorage);
		auto newCondition = condition_->handleConditionList(*player_);
		if ( newCondition )
		{
			condition_ = newCondition;
			condition_->initializeConditionList();
		}
	}
	Bot(std::shared_ptr<Player > player, Type type);
	std::shared_ptr< Player > getPlayer()
	{
		if ( !player_ )
		{
			throw rplanes::eRoomError("������ ����. ");
		}
		return player_;
	}

private:
	std::shared_ptr< BotCondition > condition_;
	std::shared_ptr< Player > player_;
};
