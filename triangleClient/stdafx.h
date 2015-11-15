#pragma once
#define _USE_MATH_DEFINES
#include < math.h >
#define PLANES_CLIENT
#include <messages.h>
#include <roomMessages.h>
#include <network.h>
#include <database.h>
#include <omp.h>
#include <thread>
#include <chrono>
#include <sstream>
#include <helpers.h>
#include <SFML/System.hpp>
#include <SFML/window.hpp>
#include <SFML/Graphics.hpp>	
#include <rstring.h>
#include <boost/archive/xml_wiarchive.hpp>