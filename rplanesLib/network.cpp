#include "network.h"

using namespace rplanes;
using namespace network;
rplanes::network::Connection::NoHandlerBehavior rplanes::network::Connection::noHandlerBehavior =
	rplanes::network::Connection::NoHandlerBehavior::THROW_EXCEPTION;

boost::system::error_code rplanes::network::Connection::accept( boost::asio::ip::tcp::acceptor & acceptor )
{
	boost::system::error_code err;
	acceptor.accept(_socket, err);
	if(!err)
		_ip = _socket.remote_endpoint().address().to_string();
	readHeaderAsync();
	return err;
}

boost::system::error_code rplanes::network::Connection::connect( boost::asio::ip::tcp::resolver::iterator & endpoint_iterator )
{
	boost::system::error_code err;
	boost::asio::connect( _socket, endpoint_iterator,  err);
	_ip = _socket.remote_endpoint().address().to_string();
	readHeaderAsync();
	return err;
}


boost::system::error_code rplanes::network::Connection::close()
{
	boost::system::error_code err;
	_socket.close(err);
	return err;
}

std::string rplanes::network::Connection::getIP()
{
	return _ip;
}


rplanes::network::Connection::Connection(boost::asio::io_service& io_service) :_socket(io_service)
{

}
