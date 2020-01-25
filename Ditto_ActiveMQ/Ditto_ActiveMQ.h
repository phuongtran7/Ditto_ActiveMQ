#pragma once

#include "Datarefs.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include <filesystem>
#include "Utility.h"

std::vector<std::unique_ptr<dataref>> topic_vector;
XPLMFlightLoopID publish_flight_loop_id{};

std::string get_config_file_path();

float publish_callback(
	float                inElapsedSinceLastCall,
	float                inElapsedTimeSinceLastFlightLoop,
	int                  inCounter,
	void* inRefcon);