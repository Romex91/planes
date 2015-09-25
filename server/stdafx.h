#pragma once
#pragma execution_character_set("utf-8")
#define  _USE_MATH_DEFINES
#include <math.h>
#include <boost/random.hpp>
#include <map>
#include <vector>
#include <fstream>
#include <thread>         // std::this_thread::sleep_for
#include <ctime>
#include <ratio>
#include <chrono>
#include <set>



#define PLANES_SERVER
#include <messages.h>
#include <parallel.h>
#include <profile.h>
#include <configuration.h>
#include <planesException.h>
#include <planesString.h>
#include <database.h>
#include <plane.h>

#include <odb/model-odb.hxx>
#include <odb/profile-odb.hxx>
#include <odb/modules-odb.hxx>

#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>	




