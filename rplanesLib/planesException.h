#pragma once
#include "planesString.h"

namespace rplanes
{
	class PlanesException : public std::exception
	{
	public:
		PlanesException(const PlanesString & string);
		virtual ~PlanesException() throw();
		virtual const char * what() const throw() override;
		const PlanesString & getString();
	private:
		const PlanesString & string_;
	};

}