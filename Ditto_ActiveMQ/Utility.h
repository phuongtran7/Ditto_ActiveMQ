#pragma once
#include "XPLMProcessing.h"
#include "XPLMPlugin.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMPlanes.h"
#include <string>
#include <cpptoml.h>

// The user�s aircraft is always index 0.
#define XPLM_USER_AIRCRAFT 0

struct aircraft_info 
{
	std::string aircraft_name;
	std::string aircraft_path;
};

std::string get_plugin_path();
aircraft_info get_loaded_aircraft();
std::string get_config_path();