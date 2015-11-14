#pragma once
#include "stdafx.h"
namespace rplanes
{
#define RPLANES_EXCEPTION(stringConstant,...) PlanesException(_rstrw(stringConstant, ##__VA_ARGS__))

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