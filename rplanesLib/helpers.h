#pragma  once
#include "stdafx.h"
namespace rplanes
{

	#define PRINT_VAR(x) std::cout <<  #x << " " << x <<std::endl

	static const rstring::_rstrw_t moduleTypesNames[] = {
		_rstrw("gun"),
		_rstrw("missile"),
		_rstrw("wing"),
		_rstrw("tail"),
		_rstrw("cockpit"),
		_rstrw("framework"),
		_rstrw("tank"),
		_rstrw("engine"),
		_rstrw("ammunition"),
		_rstrw("turret")
	};
}