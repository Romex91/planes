#pragma once
//данный файл содержит исключения
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
	//выбрасывается при запросе несуществующего модуля
	class eModuleNotFound: public planesException 
	{
	public:
		CONSTRUCTOR(eModuleNotFound);
		virtual const char* what() const
		{
			return std::string( "Модуль не содержится в базе данных. " + message).c_str();
		}
	};
	//выбрасывается при запросе несуществующей модели
	class eModelTemplateNotFound: public planesException 
	{
	public:
		CONSTRUCTOR(eModelTemplateNotFound);

		virtual const char* what() const
		{
			return std::string("Самолет не содержится в базе данных. " + message).c_str();
		}
	};
	//выбрасывается при незавершенности объекта modelTemplate
	class  eModelTemplateNotFull: public planesException 
	{
	public:
		CONSTRUCTOR(eModelTemplateNotFull);
		virtual const char* what() const
		{
			return std::string("Модель самолета не содержит необходимое количество модулей, либо подан неверный запрос. " + message).c_str();
		}
	};

	class eModulType : public planesException
	{
	public:
		CONSTRUCTOR(eModulType);
		virtual const char* what() const
		{
			return std::string("Несоответствия модуля по типу. " + message).c_str();
		}
	};

	class eMessageNotFound: public planesException
	{
	public:
		CONSTRUCTOR(eMessageNotFound);
		virtual const char* what() const
		{
			return std::string("Не найдено сообщение. " + message).c_str();
		}
	};

	class eWriteError:public planesException
	{
	public:
		CONSTRUCTOR(eWriteError);
		virtual const char* what() const
		{
			return std::string("Не удалось отправить сообщение. " + message).c_str();
		}
	};
	class eReadError:public planesException
	{
	public:
		CONSTRUCTOR(eReadError);
		virtual const char* what() const
		{
			return std::string("не удалось получить сообщение. " + message).c_str();
		}
	};
	class eClientStatusError:public planesException
	{
	public:
		CONSTRUCTOR(eClientStatusError);
		virtual const char* what() const
		{
			return std::string("Ошибка статуса клиента. " + message).c_str();
		}
	};
	class eLoginFail:public planesException
	{
	public:
		CONSTRUCTOR(eLoginFail);
		virtual const char* what() const
		{
			return std::string("Ошибка авторизации. " + message).c_str();
		}
	};
	class eClientsOutOfRange:public planesException
	{
	public:
		CONSTRUCTOR(eClientsOutOfRange);
		virtual const char* what() const
		{
			return std::string("Выход за границы списка клиентов. " + message).c_str();
		}
	};
	class eClientNotConnected:public planesException
	{
	public:
		CONSTRUCTOR(eClientNotConnected);
		virtual const char* what() const
		{
			return std::string("Попытка обращения к отключенному соединению. " + message).c_str();
		}
	};
	class eProfileError:public planesException
	{
	public:
		CONSTRUCTOR(eProfileError);
		virtual const char* what() const
		{
			return std::string("Ошибка при работе с профилем. " + message).c_str();
		}
	};
	class eClientConnectionFail:public planesException
	{
	public:
		CONSTRUCTOR(eClientConnectionFail);
		virtual const char* what() const
		{
			return std::string("Невозможно подключить клиента. " + message).c_str();
		}
	};
	class eRoomError:public planesException
	{
	public:
		CONSTRUCTOR(eRoomError);
		virtual const char* what() const
		{
			return std::string("Ошибка комнаты. " + message).c_str();
		}
	};

	#undef CONSTRUCTOR
}
