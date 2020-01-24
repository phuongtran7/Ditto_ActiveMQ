#pragma once

#include "Datarefs.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include <filesystem>
#include "Utility.h"

std::vector<std::unique_ptr<dataref>> topic_vector;
XPLMFlightLoopID data_flight_loop_id{};

std::string get_config_file_path();

float data_callback(
	float                inElapsedSinceLastCall,
	float                inElapsedTimeSinceLastFlightLoop,
	int                  inCounter,
	void* inRefcon);