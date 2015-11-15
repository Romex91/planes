#pragma once
#include "stdafx.h"

#define RPLANES_EXCEPTION(stringConstant,...) rplanes::PlanesException(_rstrw(stringConstant, ##__VA_ARGS__))

namespace rplanes
{

	class PlanesException : public std::exception
	{
	public:
		PlanesException(const rstring::_rstrw_t & string);
		virtual ~PlanesException() throw();
		virtual const char * what() const throw() override;
		const rstring::_rstrw_t & getString() const;
	private:
		rstring::_rstrw_t _string;
	};

}