#pragma once
#include "network.h"

//max count of registered messages
#define RPLANES_MAX_MESSAGE_ID 100

//macro to place in message body
#define RPLANES_MESSAGE_ID(messageId)\
	public:\
	virtual MessageId getId() const override { return id; }\
enum { id = messageId }; 


#define RPLANES_REGISTER_MESSAGE_FOR_ARCHIVE(classname, archive)\
	template<>\
	inline std::shared_ptr<rplanes::network::details::MSerializerBase>\
		rplanes::network::details::registeredSerializer<classname::id, archive>(archive & ar)\
{\
	return std::shared_ptr<MSerializerBase>(new details::MSerializer<classname, archive>(ar));\
}\

#define RPLANES_REGISTER_MESSAGE(classname) \
	RPLANES_REGISTER_MESSAGE_FOR_ARCHIVE(classname, boost::archive::binary_iarchive)\
	RPLANES_REGISTER_MESSAGE_FOR_ARCHIVE(classname, boost::archive::text_iarchive)\
	RPLANES_REGISTER_MESSAGE_FOR_ARCHIVE(classname, boost::archive::binary_oarchive)\
	RPLANES_REGISTER_MESSAGE_FOR_ARCHIVE(classname, boost::archive::text_oarchive)


namespace rplanes
{
	namespace network
	{
		typedef unsigned short MessageId;


		class MessageBase
		{
		public:
			//type-specific id
			//used when deserializing
			virtual MessageId getId() const = 0;
			virtual ~MessageBase() {};
		};

		//compile-time message registering magic
		namespace details {

			template<class _Message, class _Archive>
			typename std::enable_if<_Archive::is_saving::value>::type writeImpl(const _Message & message, _Archive & ar)
			{
				ar << message;
			}

			template<class _Message, class _Archive>
			typename std::enable_if<!_Archive::is_saving::value>::type writeImpl(const _Message & message, _Archive & ar)
			{
				throw RPLANES_EXCEPTION("{0} is not saving archive", typeid(_Archive).name());
			}

			//message serializer base
			class MSerializerBase
			{
			public:
				virtual std::shared_ptr<MessageBase> read() = 0;
				virtual void write(const MessageBase & message) = 0;
				virtual ~MSerializerBase() {};
			};

			//archive-specific message serializer
			template<class _Message, class _Archive >
			class MSerializer : public MSerializerBase
			{
			public:
				MSerializer(_Archive & archive) : ar(archive) {}

				virtual std::shared_ptr<MessageBase> read() override
				{
					if (!_Archive::is_loading::value)
						throw RPLANES_EXCEPTION("{0} is not loading archive.", typeid(_Archive).name());
					_Message mess;
					ar & mess;
					return std::shared_ptr<MessageBase>(new _Message(mess));
				}

				virtual void write(const MessageBase & message) override
				{
					writeImpl(dynamic_cast<const _Message &>(message), ar);
				}
			private:
				_Archive & ar;
			};

			//jump table of registered serializers
			template<class _Archive, MessageId...ids>
			std::shared_ptr<MSerializerBase> selectRegisteredSerializer
				(MessageId id, _Archive & ar, std::integer_sequence<MessageId, ids...>)
			{
				using base_maker = std::shared_ptr<MSerializerBase>(*)(_Archive & ar);
				static const base_maker makers[]{
					registeredSerializer<ids>...
				};
				return makers[id](ar);
			}


			//to register a message create specializations of this method using the message ID and all the archives you are going to use
			//see RPLANES_REGISTER_MESSAGE
			template<MessageId _Id, class _Archive>
			std::shared_ptr<MSerializerBase> registeredSerializer(_Archive & ar)
			{
				throw RPLANES_EXCEPTION("unregistered message {0} for archive '{1}'", _Id, typeid(_Archive).name());
			}

			//returns registered serializer by runtime id
			template<class _Archive>
			std::shared_ptr<MSerializerBase> getMessageSerializer(MessageId id, _Archive & ar)
			{
				if (id >= RPLANES_MAX_MESSAGE_ID)
					throw RPLANES_EXCEPTION("message id {0} is bigger then allowed {1}", id, RPLANES_MAX_MESSAGE_ID);
				return details::selectRegisteredSerializer(id, ar,
					std::make_integer_sequence<MessageId, RPLANES_MAX_MESSAGE_ID>{});
			}
		}


		template<class _Archive>
		std::shared_ptr<MessageBase> readRegisteredMessage(_Archive & ar)
		{
			MessageId id;
			ar >> id;
			auto serializer = details::getMessageSerializer(id, ar);
			return serializer->read();
		}


		template<class _Archive>
		void writeRegisteredMessage(const MessageBase & mess, _Archive & ar)
		{
			ar << mess.getId();
			auto serializer = details::getMessageSerializer(mess.getId(), ar);
			return serializer->write(mess);
		}

	}
}

