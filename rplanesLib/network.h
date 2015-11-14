#pragma once
#include "stdafx.h"
#include "planesException.h"
#include "parallel.h"
class Client;

namespace rplanes
{
	namespace network
	{
//max count of registered messages
#define RPLANES_MAX_MESSAGE_ID 100

		typedef unsigned short MessageId;


		class MessageBase
		{
		public:
			//type-specific id
			//used when deserializing
			virtual MessageId getId() const = 0;
			virtual ~MessageBase(){};
		};

		namespace details {
			template<MessageId _Id, class _Archive>
			std::shared_ptr<MessageBase> registeredReadFunction(_Archive & ar)
			{
				throw RPLANES_EXCEPTION("unregistered message {0} for iostream '{1}'", _Id, typeid(_Archive).name());
			}

			template<class _Archive, MessageId...ids>
			std::shared_ptr<MessageBase> selectRegisteredReadFunction
				(MessageId id, _Archive & ar, std::integer_sequence<MessageId, ids...>)
			{
				using base_maker = std::shared_ptr<MessageBase>(*)(_Archive & ar);
				static const base_maker makers[]{
					registeredReadFunction<ids>...
				};
				return makers[id](ar);
			}

			template <class _Message, class _Archive>
			std::shared_ptr<MessageBase> readMessageImpl(_Archive & ar)
			{
				_Message mess;
				ar >> mess;
				return std::shared_ptr<MessageBase>(new _Message(mess));
			}
		}

		template<class _Archive>
		inline std::shared_ptr<MessageBase> readMessage(MessageId id, _Archive & ar)
		{
			if (id >= RPLANES_MAX_MESSAGE_ID)
				throw RPLANES_EXCEPTION("message id {0} is bigger then allowed {1}", id, RPLANES_MAX_MESSAGE_ID);
			return details::selectRegisteredReadFunction(id, ar,
				std::make_integer_sequence<MessageId, RPLANES_MAX_MESSAGE_ID>{});
		}

		//contains a bit of compile-time magic and asynch insanity
		class Connection
		{
		public:
			typedef boost::archive::binary_oarchive OArchive;
			typedef boost::archive::binary_iarchive IArchive;

			enum { headerLength = sizeof(size_t) * 2};

			enum class NoHandlerBehavior{
				THROW_EXCEPTION,
				DO_NOTHING
			};

			Connection(boost::asio::io_service& io_service);

			template<class _Message>
			void setHandler(std::function<void(const _Message &)> handler)
			{
				_handlers[_Message::id] = [handler](const MessageBase & message) {
					handler(dynamic_cast<const _Message &>(message));
				};
			}

			template<class _Message>
			void sendMessage( const _Message & message )
			{
				std::vector<boost::asio::const_buffer> buffers;

				//serializing data
				std::ostringstream archive_stream;
				OArchive archive(archive_stream, boost::archive::no_header);
				MessageId id = _Message::id;
				archive << id;
				archive << message;

				//first send the serialized data size
				std::ostringstream data_header_stream;
				data_header_stream << std::setw(headerLength) << std::hex << archive_stream.str().size();
				buffers.push_back(boost::asio::buffer(data_header_stream.str()));
				//sending message
				buffers.push_back(boost::asio::buffer(archive_stream.str()));

				_socket.non_blocking(false);
				boost::system::error_code error;
				boost::asio::write(_socket, buffers, error);
				if (error)
				{
					throw RPLANES_EXCEPTION("Failed sending message. {0}", error.message());
				}
			}

			std::shared_ptr<MessageBase> handleMessage()
			{
				std::istringstream messageStream;
				{
					MutexLocker l(_mutex);
					if (_pendingMessages.size() == 0)
						return nullptr;
					auto frontValue = _pendingMessages.front();
					messageStream = std::istringstream(std::string(&frontValue[0], frontValue.size()));
					_pendingMessages.pop();
				}

				IArchive archive(messageStream, boost::archive::no_header);
				MessageId id;
				archive >> id;
				auto message = readMessage(id, archive);

				if (_handlers.count(id) == 0) {
					if (noHandlerBehavior == NoHandlerBehavior::DO_NOTHING)
						return message;
					throw RPLANES_EXCEPTION("no handler for the message {0}", id);
				}
				_handlers[id](*message);
				return message;
			}

			boost::system::error_code accept(boost::asio::ip::tcp::acceptor & acceptor);
			boost::system::error_code connect(boost::asio::ip::tcp::resolver::iterator & endpoint_iterator);
			boost::system::error_code close();
			std::string getIP();

			static NoHandlerBehavior noHandlerBehavior;

		protected:
			std::string _ip;
			boost::asio::ip::tcp::socket _socket;
			std::map<MessageId, std::function<void(const MessageBase &)>> _handlers;
			std::queue<std::vector<char>> _pendingMessages;

			//shared resource is _pendingMessages
			Mutex _mutex;

			//buffers for async read methods
			char _headerBuffer[headerLength];
			std::vector<char> _messageBuffer;
			void readHeaderAsync() 
			{
				boost::asio::async_read(_socket,
					boost::asio::buffer(_headerBuffer, headerLength),
					[this](boost::system::error_code ec, std::size_t /*length*/)
				{
					size_t messageLength = 0;
					std::istringstream is(std::string(_headerBuffer, headerLength));
					if (!ec && (is >> std::hex >> messageLength))
					{
						readMessageAsync(messageLength);
					}
					else 
					{
						_socket.close();
					}
				});
			}

			void readMessageAsync(size_t messageLength)
			{
				_messageBuffer.resize(messageLength);
				boost::asio::async_read(_socket, boost::asio::buffer(_messageBuffer),
					[this, messageLength](boost::system::error_code ec, std::size_t uploadedLength)
				{
					if (!ec && uploadedLength == messageLength) {
						MutexLocker l(_mutex);
						_pendingMessages.push(_messageBuffer);
						readHeaderAsync();
					}
					else 
					{
						_socket.close();
					}
				});
			}

		};

	}
}

#define RPLANES_MESSAGE_ID(messageId)\
	public:\
	virtual MessageId getId() const override { return id; }\
enum { id = messageId }; \


#define RPLANES_REGISTER_MESSAGE(classname, archive)\
	template<>\
	inline std::shared_ptr<rplanes::network::MessageBase>\
		rplanes::network::details::registeredReadFunction<classname::id, archive>(archive & ar)\
{\
	return readMessageImpl<classname, archive>(ar);\
}\

#define RPLANES_REGISTER_MESSAGE_ALL_ARCHIVES(classname) \
	RPLANES_REGISTER_MESSAGE(classname, boost::archive::binary_iarchive)\
	RPLANES_REGISTER_MESSAGE(classname, boost::archive::text_iarchive)

