#pragma once
#include "stdafx.h"
namespace rplanes
{
	class PlanesException : public std::exception
	{
	public:
		PlanesException(const rstring::_rstrw_t & string);
		virtual ~PlanesException() throw();
		virtual const char * what() const throw() override;
		const rstring::_rstrw_t & getString();
	private:
		const rstring::_rstrw_t & _string;
	};

}