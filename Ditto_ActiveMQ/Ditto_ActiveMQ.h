#pragma once

#include "PublishTopic.h"
#include "SubscribeTopic.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include <filesystem>
#include "Utility.h"

std::vector<std::unique_ptr<PublishTopic>> publish_topic_vector;
std::vector<std::unique_ptr<SubscribeTopic>> subscribe_topic_vector;

XPLMFlightLoopID publish_flight_loop_id{};
XPLMFlightLoopID subscribe_flight_loop_id{};

std::string get_config_file_path();

float publish_callback(
	float                inElapsedSinceLastCall,
	float                inElapsedTimeSinceLastFlightLoop,
	int                  inCounter,
	void* inRefcon);

float subscribe_callback(
	float                inElapsedSinceLastCall,
	float                inElapsedTimeSinceLastFlightLoop,
	int                  inCounter,
	void* inRefcon);