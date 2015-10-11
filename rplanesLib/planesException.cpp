#include "planesException.h"

rplanes::PlanesException::PlanesException(const rstring::_rstrw_t & string) : _string(string)
{

}

rplanes::PlanesException::~PlanesException()
{

}

const char * rplanes::PlanesException::what() const
{
	try {
		return rstring::helpers::convert<std::wstring, std::string> (_string.str()).c_str();
	}
	catch (...) {
		return "PlanesException::what() failed";
	}
}

const rstring::_rstrw_t & rplanes::PlanesException::getString() const
{
	return _string;
}
