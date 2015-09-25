#include "stdafx.h"
#include "planesException.h"


rplanes::PlanesException::PlanesException(const PlanesString & string) : string_(string)
{

}

rplanes::PlanesException::~PlanesException()
{

}

const char * rplanes::PlanesException::what() const
{
	try {
		return string_.str().c_str();
	}
	catch (...) {
		return "PlanesException::what() failed";
	}
}

const rplanes::PlanesString & rplanes::PlanesException::getString()
{
	return string_;
}
