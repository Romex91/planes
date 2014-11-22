#pragma  once
//������ ���� �������� ������ �������(�������������� ��������� ��������), � ����� ������ ���� � ���������� ������
#include "stdafx.h"
#include "hitzone.h"
namespace rplanes
{
	enum GunType {GUN1, GUN2, GUN3, GUN4, CANNON1, CANNON2, CANNON3, CANNON4}; //��� ������ ��������. ��� ���� ����� ������, ��� ����� ����� ����� ��������. ���������� �������� ��� ��������(shop, not magazine).
	enum EngineType {SCREW, JET};
	enum ModuleType {GUN, MISSILE, WING, TAIL, CABINE, FRAMEWORK, TANK, ENGINE, AMMUNITION, TURRET};

	namespace planedata
	{
		// ������� ������� ������, ����� ����������� �� ��������
		class Defect 
		{
		public:
			// ��� timer >= permanentTimerValue ������� ����������
			Defect(  float timer = -1.f ):  timer_(timer), updateRequired_(true) 
			{}
			//��������� �� ������
			operator bool()const;
			//�������� �� ����������� ������������
			bool isPermanent()const;
			//��������� �������� �������. ���������� true ���� ������ ��� ��������
			bool decrementTimer( float offset );
			//��������� �������� �������. ��� timer >= permanentTimerValue ������� ����������
			void incrementTimer( float offset ); 
			//�������� �������� �������
			void reset( float permanentTimerValue );
			//��������� ��� �� ������ ���������/������� �� ������� ���������� ������  checkUpdateNeed
			bool checkUpdateNeed();
		private:
			// ������������� ���������� ����������� ���������� ��������
			bool updateRequired_; 
			float timer_, permanentTimerValue_;
		};

		class ModuleHP
		{
		public:
			friend class Module;
			//�������, �� ������� ��������� ��������� ��������� ������
			enum ChangeReason
			{
				HIT,
				FIRE,
				REPAIR
			};

			//������ �� ������
			bool isDefected()const;

			//��������� ���� �� �������� ���������(���������, ������ �������) c� ������� ���������� ������ checkConditionChange
			bool checkConditionChange();
			//��������� ��� �� ������ ������/������� �� ������� ���������� ������ checkDefectUpdate
			bool checkDefectUpdate();
			//�������� ������ �������. ������ ����� ���� �������
			void decrementDefectTimer(float frameTime);
			//������� ���������� ��������� ���������
			ChangeReason getReason()const;
			//����� �������� ��������� ����
			size_t getLastHitClient()const;
			//����������� ��
			operator int()const;
		private:
			void reset(size_t clientID, int hpMax, float defectChance, float permanentTimerValue, float timerValueSigma, float timerValueMean);

			//������� ������
			void breakDown(ChangeReason reason, float timerVal);

			//��������� ������. ������ ����� ���������
			void damage(float damageVal, size_t clientID, ChangeReason reason);

			//����������� ������� ��� ��������� ����������� �� �������� ���������
			float defectChance_;

			float
				timerValueSigma_,
				timerValueMean_;


			//������������ �������� ���������
			int hpMax_;
			//������� ����������
			int hp_;
			//���������� ������ �� ������
			Defect defect_;
			//����� �������� �������� ����
			size_t lastHitClient_;
			//���� �� �������� ��������� ������
			bool hpUpdated_;
			//������� ��������� ���������
			ChangeReason changeReason_;
		};

#pragma db object polymorphic
		class Module
		{
		public:
#pragma db id
			std::string name;

			int price;
			//������������ ���������
			int hpMax;
			//��� � �����������
			int weigth;
			//� ����������� ������� �����
			float armor; 

			float 
				permanentDefectTimerValue,
				defectTimerValueSigma,
				defectTimerValueMean;

			//����������� ������ �� ����� ��� ��������� ����������� �� �������� ���������
			float defectChance; 

			//������� ���������
#pragma db transient
			ModuleHP hp;

			//���� �����������
#pragma db transient
			serverdata::HitZone hitZone;

			//����� � ��������
#pragma db transient
			size_t pos; 


			void reload( size_t clientID );


			virtual ModuleType getType() = 0;


			virtual ~Module();;


			//������� ������
			void breakDown(ModuleHP::ChangeReason reason, float timerVal)
			{
				hp.breakDown(reason, timerVal);
			}

			//��������� ������. ������ ����� ���������
			void damage(float damageVal, size_t clientID, ModuleHP::ChangeReason reason)
			{
				hp.damage(damageVal, clientID, reason);
				if ( hp.isDefected() )
				{
					if (getType() == AMMUNITION || getType() == TANK)
					{
						hp.damage( hpMax , clientID, ModuleHP::FIRE);
					}
				}
			}
		protected:
			virtual void derv_reload();

		};

#pragma db object
		class Gun : public Module
		{
		public:
			Gun();
			ModuleType getType();

			//������ ��� ���������� ��������
#pragma db transient
			float timer;

			size_t caliber;
			int damage;
			//��� ��������
			GunType type;		
			float penetration,	
				shootRate,		
				speed,			
				acceleration,
				accuracy,
				impact;


			//���� �������� ��� �����������
			float defectShootRate();

			//�������� �� ����� � �������� gt;
			bool isSuitable( GunType gunType );

			float getMaxDistance( float planeSpeed );

			//�������� �����, ����� ������� ���������� ������ ��������� ����
			float getHitTime( float shootingDistance, float planeSpeed);

			//�������� ��������� ������������ ��������, ����������� ��� �������� �� ��������� ���������
			float getSpeedZ(float shootingDistance, float planeSpeed, float gunHeight = 0.f);

		protected:
			void derv_reload();
		private:
			void init();
		};

#pragma db object
		class Missile : public Module
		{
		public:
			ModuleType getType();
			bool isEmpty;
			// ���� � ������� ��� ������
			int damage; 
			float radius,
				speed,
				acceleration,
				ttl,
				accuracy;

			float getMaxDistance( float planeSpeed );
			//�������� �����, ����� ������� ���������� ������ ��������� ����
			float getHitTime( float shootingDistance, float planeSpeed );
			//�������� ��������� ������������ ��������, ����������� ��� �������� �� ��������� ���������
			float getSpeedZ( float shootingDistance, float planeSpeed );
		protected:
			void derv_reload();
		};

#pragma db object
		class Wing : public Module
		{
		public:
			ModuleType getType();
			float surface;
			float rollSpeed;
			float defectSurfase();
		};
#pragma db object
		class Tail : public Module
		{
		public:
			ModuleType getType();
			float mobilityFactor;
			float defectMobilityFactor();
		};
#pragma db object
		class Cabine : public Module
		{
		public:
			ModuleType getType();
		};
#pragma db object
		class Framework : public Module
		{
		public:
			ModuleType getType();
			float Vmax, Vmin, Gmax;
			float defectVmax();
		};
#pragma db object
		class Tank : public Module
		{
		public:
			ModuleType getType();
			//�������
			float capacity;
			//�����
#pragma db transient
			float fuel;		
		protected:
			void derv_reload()
			{
				fuel = capacity;
			}
		};
#pragma db object
		class Engine : public Module
		{
		public:
			ModuleType getType();
			Engine()
			{
				init();
			}
			EngineType type;
			float maxPower;
			float fuelIntake;

			//������������� ���������
			float
				maxTemperature,					//����������� ��� ����������� �������� � ������������ ����
				minTemperature,					//����������� ��� ������������ �������� � ����������� ����  
				declaredMaxDt,					//����������� ���������� ����������� �����������
				criticalTemperature,			//�����������, ��� ������� ��������� ������������
				heatingFactor,					//������� ���������� ���������
				cooldownFactor;					//������� ���������� ���������


#pragma db transient
			float temperature;
#pragma db transient
			float stabTemperature;
#pragma db transient
			float dt;
#pragma db transient
			float maxDt;
#pragma db transient
			float temperatureFactor;
			float defectTMax();
		protected:
			void derv_reload();

		private:
			void init();
		};
#pragma db object
		class Ammunition : public Module
		{
		public:
			ModuleType getType();
			size_t caliber;
			size_t maxCapacity;
#pragma db transient
			size_t capacity;
		protected:
			void derv_reload()
			{
				capacity = maxCapacity;
			}
		};
#pragma  db object
		class Turret : public Module
		{
		public:
			ModuleType getType();

			std::string gunName;
			size_t nGuns;

#pragma db transient
			Gun gun;


			//���������� ����� �������
#pragma db transient
			float gunShift;

#pragma db transient
			PointXYZ gunsPosition;

			//������ ��������
#pragma db transient
			float sector;
			//��������� ����������� ������
#pragma db transient 
			float startAngle;


			//��������������� ������

#pragma  db transient
			PointXY aimError;
#pragma db transient
			float aimTimer;
#pragma db transient
			float shootTimer;
#pragma db transient
			float cooldownTimer;
			//������� ��������� ��������
#pragma db transient
			float aimDistance;
			//������� ���� ������
#pragma db transient 
			float aimAngle;
#pragma db transient
			bool isShooting;

			std::vector< PointXYZ > getRotatedGunsPositions( float angle, float roll );

			Turret();

			virtual void derv_reload();

		private:
		};
	}

}

