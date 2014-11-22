#pragma once
//������ ���� �������� ����������
#include  "stdafx.h"

namespace rplanes
{

	class planesException : public std::exception
	{
	protected:
		std::string message;
	public:
		planesException(){};
		planesException( std::string text ):message(text){};
		virtual const char* what() const
		{
			return message.c_str();
		};
	private:
	};
#define CONSTRUCTOR(x) x (std::string text=""): planesException(text){}
	//������������� ��� ������� ��������������� ������
	class eModuleNotFound: public planesException 
	{
	public:
		CONSTRUCTOR(eModuleNotFound);
		virtual const char* what() const
		{
			return std::string( "������ �� ���������� � ���� ������. " + message).c_str();
		}
	};
	//������������� ��� ������� �������������� ������
	class eModelTemplateNotFound: public planesException 
	{
	public:
		CONSTRUCTOR(eModelTemplateNotFound);

		virtual const char* what() const
		{
			return std::string("������� �� ���������� � ���� ������. " + message).c_str();
		}
	};
	//������������� ��� ��������������� ������� modelTemplate
	class  eModelTemplateNotFull: public planesException 
	{
	public:
		CONSTRUCTOR(eModelTemplateNotFull);
		virtual const char* what() const
		{
			return std::string("������ �������� �� �������� ����������� ���������� �������, ���� ����� �������� ������. " + message).c_str();
		}
	};

	class eModulType : public planesException
	{
	public:
		CONSTRUCTOR(eModulType);
		virtual const char* what() const
		{
			return std::string("�������������� ������ �� ����. " + message).c_str();
		}
	};

	class eMessageNotFound: public planesException
	{
	public:
		CONSTRUCTOR(eMessageNotFound);
		virtual const char* what() const
		{
			return std::string("�� ������� ���������. " + message).c_str();
		}
	};

	class eWriteError:public planesException
	{
	public:
		CONSTRUCTOR(eWriteError);
		virtual const char* what() const
		{
			return std::string("�� ������� ��������� ���������. " + message).c_str();
		}
	};
	class eReadError:public planesException
	{
	public:
		CONSTRUCTOR(eReadError);
		virtual const char* what() const
		{
			return std::string("�� ������� �������� ���������. " + message).c_str();
		}
	};
	class eClientStatusError:public planesException
	{
	public:
		CONSTRUCTOR(eClientStatusError);
		virtual const char* what() const
		{
			return std::string("������ ������� �������. " + message).c_str();
		}
	};
	class eLoginFail:public planesException
	{
	public:
		CONSTRUCTOR(eLoginFail);
		virtual const char* what() const
		{
			return std::string("������ �����������. " + message).c_str();
		}
	};
	class eClientsOutOfRange:public planesException
	{
	public:
		CONSTRUCTOR(eClientsOutOfRange);
		virtual const char* what() const
		{
			return std::string("����� �� ������� ������ ��������. " + message).c_str();
		}
	};
	class eClientNotConnected:public planesException
	{
	public:
		CONSTRUCTOR(eClientNotConnected);
		virtual const char* what() const
		{
			return std::string("������� ��������� � ������������ ����������. " + message).c_str();
		}
	};
	class eProfileError:public planesException
	{
	public:
		CONSTRUCTOR(eProfileError);
		virtual const char* what() const
		{
			return std::string("������ ��� ������ � ��������. " + message).c_str();
		}
	};
	class eClientConnectionFail:public planesException
	{
	public:
		CONSTRUCTOR(eClientConnectionFail);
		virtual const char* what() const
		{
			return std::string("���������� ���������� �������. " + message).c_str();
		}
	};
	class eRoomError:public planesException
	{
	public:
		CONSTRUCTOR(eRoomError);
		virtual const char* what() const
		{
			return std::string("������ �������. " + message).c_str();
		}
	};

	#undef CONSTRUCTOR
}
