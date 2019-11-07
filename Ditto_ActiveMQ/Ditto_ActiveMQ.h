#pragma once

// Manually apply fix for numeric_limits<T>::max() to 
//https://github.com/google/flatbuffers/commit/a5ca8bee4d56df1588b21d667135be351d6c0e75

#include "ActiveMQ.h"
#include "Datarefs.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"

dataref new_data;
XPLMFlightLoopID data_flight_loop_id{};
XPLMFlightLoopID retry_flight_loop_id{};
std::unique_ptr<Producer> producer;
int activemq_counter_value;
XPLMDataRef ActiveMQ = nullptr;

struct activemq_config {
	std::string broker_address;
	std::string topic;
};

float data_callback(
	float                inElapsedSinceLastCall,
	float                inElapsedTimeSinceLastFlightLoop,
	int                  inCounter,
	void* inRefcon);

float retry_callback(
	float                inElapsedSinceLastCall,
	float                inElapsedTimeSinceLastFlightLoop,
	int                  inCounter,
	void* inRefcon);

activemq_config get_activemq_config();
void init_activemq();
int GetActiveMQCounter(void* inRefcon);
void SetActiveMQcounter(void* inRefcon, int inValue);