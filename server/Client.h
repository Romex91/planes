#pragma once
#include "stdafx.h"
#include "room.h"

using namespace rplanes::network;


//���������� ������ �� id
bool clientIsInRoom(size_t clientID );
//�������� ��������� � ������� �� id
size_t convertIDToPos( size_t clienID );
size_t convertPosToID( size_t posInVector, bool inRoom  );

//����� �������.
//�����������:
//������� �� ����� ����� ������ � ������ ��������. ��������� ������� �� ������ ������ �� ��������� ������ ��������
//��������� � ������� ����� ���� ������������ ������ ���������������
class Client
{
public:
	friend class Server;

	Client( boost::asio::io_service& io_service , size_t clientID = 0 );
	~Client();
	void setID( size_t id );

	void sendMessage( Message & mess );

	void setControllable( rplanes::serverdata::Plane::ControllableParameters controllable );

	void login( std::string name, std::string password );

	void logout();

	void prepareRoomExit();

	ClientStatus getStatus();

	rplanes::playerdata::Profile & profile();

private:

	//��������� ��������� ������ (������� ��������� � �.�.)
	void sendRoomMessages();

	//����������� �� �������� ����� ����� ���������� ������� � ������� �������
	void joinRoom( Room& room, size_t planeNo );

	//����������� �� ����� ������ ��� ������ �� ���
	void exitRoom();

	struct ProfilesInfo
	{
		std::set<std::string> loginedProfiles;
		Mutex Mutex;
		size_t clientsCount;
		ProfilesInfo()
		{
			clientsCount = 0;
		}
	};

	static ProfilesInfo profilesInfo_;
	size_t id_; 
	float disconnectTimer_;
	ClientStatus status_;
	Connection connection_;

	rplanes::playerdata::Profile profile_;
	std::shared_ptr< Player > player_;
};
