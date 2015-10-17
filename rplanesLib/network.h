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

		template<class _Message, class _Archive>
		class MessageSerializer
		{
		public:
			MessageSerializer(_Archive & archive) : _ar(archive){}

			std::shared_ptr<MessageBase> read()
			{
				_Message message;
				_ar >> message;
				return std::make_shared<MessageBase>(message);
			}
			void write( MessageBase & mess )
			{
				_Message & message = dynamic_cast<_Message & >(mess);
				_ar << message;
			}
		private:
			_Archive & _ar;
		};
		
		

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
					throw PlanesException(_rstrw("Failed sending message. {0}", error.message()));
				}
			}

			std::shared_ptr<MessageBase> handleMessage()
			{
				std::istringstream messageStream;
				{
					MutexLocker l(_mutex);
					auto frontValue = _pendingMessages.front();
					messageStream = std::istringstream(std::string(&frontValue[0], frontValue.size()));
					_pendingMessages.pop();
				}

				IArchive archive(messageStream, boost::archive::no_header);
				MessageId id;
				archive >> id;
				return handleMessage_selectSpecialization(id, archive);
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

			//read a message with a specific id from the socket
			//calls the id-specific specialization for the registered message type
			template<MessageId _Left = 0, MessageId _Right = RPLANES_MAX_MESSAGE_ID>
			std::shared_ptr<MessageBase> handleMessage_selectSpecialization(MessageId messageId, IArchive & ar)
			{
				if (_Left == messageId)
					return handleMessage_registered<_Left>(ar);
				else
					return handleMessage_selectSpecialization<_Left + 1>(messageId, ar);
			}

			//to register a new message add a specialization of this method containing the call to readMessageImpl
			//see the RPLANES_REGISTER_MESSAGE macro
			template <MessageId _MessageId>
			std::shared_ptr<MessageBase> handleMessage_registered(IArchive & ar)
			{
				throw PlanesException(_rstrw("unregistered message id {0}", _MessageId));
			}

			//read and handle a message of the specific type
			template <class _Message>
			std::shared_ptr<MessageBase> handleMessageImpl(IArchive & ar)
			{
				_Message mess;
				ar >> mess;
				if (_handlers.count(_Message::id) == 0) {
					if (noHandlerBehavior == NoHandlerBehavior::DO_NOTHING)
						return std::shared_ptr<MessageBase> (new _Message(mess));
					throw PlanesException(_rstrw("no handler for the message {0}", _Message::id));
				}
				_handlers[_Message::id](mess);
				return std::shared_ptr<MessageBase>(new _Message(mess));
			}
		};

		template <>
		inline std::shared_ptr<MessageBase> Connection::handleMessage_selectSpecialization<RPLANES_MAX_MESSAGE_ID + 1>(MessageId messageId, IArchive & ar)
		{
			throw PlanesException(_rstrw("unregistered message id {0}", RPLANES_MAX_MESSAGE_ID + 1));
		}

	}
}

#define RPLANES_MESSAGE_ID(messageId)\
	public:\
	virtual MessageId getId() const override { return id; }\
enum { id = messageId }; \

#define RPLANES_REGISTER_MESSAGE(classname)\
	template<>\
	inline std::shared_ptr<rplanes::network::MessageBase> rplanes::network::Connection::handleMessage_registered<classname::id>(rplanes::network::Connection::IArchive & ar)\
{\
	return handleMessageImpl<classname>(ar); \
}\

