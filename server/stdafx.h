#pragma once
#define  _USE_MATH_DEFINES
#include <math.h>
#include <boost/random.hpp>
#include <boost/archive/xml_wiarchive.hpp>
#include <boost/lockfree/queue.hpp>
#include <map>
#include <vector>
#include <fstream>
#include <thread>         // std::this_thread::sleep_for
#include <ctime>
#include <ratio>
#include <chrono>
#include <set>
#include <functional>

#include <messages.h>
#include <roomMessages.h>
#include <parallel.h>
#include <profile.h>
#include <configuration.h>
#include <planesException.h>
#include <database.h>
#include <plane.h>
#include <network.h>

#include <odb/model-odb.hxx>
#include <odb/profile-odb.hxx>
#include <odb/modules-odb.hxx>

#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>	




