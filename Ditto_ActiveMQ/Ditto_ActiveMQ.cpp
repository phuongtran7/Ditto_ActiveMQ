#define NOMINMAX

#include "ActiveMQ.h"
#include "Datarefs.h"

#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"

dataref new_data;
XPLMFlightLoopID data_flight_loop_id{};
XPLMFlightLoopID retry_flight_loop_id{};
std::shared_ptr<Producer> producer;


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

PLUGIN_API int XPluginStart(
	char *		outName,
	char *		outSig,
	char *		outDesc)
{
	std::string name = "ActiveMQ Data Extractor";
	std::string signature = "phuong.x-plane.activemq.extractor";
	std::string description = "X-Plane Plugin that send message to an ActiveMQ Broker";

	strcpy_s(outName, name.length() + 1, name.c_str());
	strcpy_s(outSig, signature.length() + 1, signature.c_str());
	strcpy_s(outDesc, description.length() + 1, description.c_str());

	activemq::library::ActiveMQCPP::initializeLibrary();

	producer = std::make_shared<Producer>("failover:(tcp://192.168.72.249:61616)", "XP-A320");

	return 1;
}

PLUGIN_API void	XPluginStop(void)
{
	XPLMDebugString("Stopping Ditto.\n");
}

PLUGIN_API void XPluginDisable(void) {
	new_data.empty_list();
	if (data_flight_loop_id != nullptr) {
		XPLMDestroyFlightLoop(data_flight_loop_id);
	}
	if (retry_flight_loop_id != nullptr) {
		XPLMDestroyFlightLoop(retry_flight_loop_id);
	}

	producer->close();
	producer.reset();

	activemq::library::ActiveMQCPP::shutdownLibrary();

	XPLMDebugString("Disabling Ditto.\n");
}
PLUGIN_API int  XPluginEnable(void)  {
	if (!new_data.get_status()) {
		if (new_data.init()) {

			producer->run();

			if (new_data.get_not_found_list_size() != 0) {
				// Register a new flight loop to retry finding datarefs
				XPLMCreateFlightLoop_t retry_params = { sizeof(XPLMCreateFlightLoop_t), xplm_FlightLoop_Phase_AfterFlightModel, retry_callback, nullptr };
				retry_flight_loop_id = XPLMCreateFlightLoop(&retry_params);
				if (retry_flight_loop_id != nullptr)
				{
					XPLMScheduleFlightLoop(retry_flight_loop_id, 5, true);
				}
			}

			XPLMCreateFlightLoop_t data_params = { sizeof(XPLMCreateFlightLoop_t), xplm_FlightLoop_Phase_AfterFlightModel, data_callback, nullptr };
			data_flight_loop_id = XPLMCreateFlightLoop(&data_params);
			if (data_flight_loop_id == nullptr)
			{
				XPLMDebugString("Cannot create flight loop. Exiting Ditto.\n");
				return 0;
			}
			else {
				XPLMScheduleFlightLoop(data_flight_loop_id, -1, true);
			}
		}
		else {
			XPLMDebugString("Cannot find \"Datarefs.toml\" or \"Datarefs.toml\" is empty. Exiting.\n");
			return 0;
		}
	}

	XPLMDebugString("Enabling Ditto.\n");
	return 1;
}
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam) { }

float data_callback(float inElapsedSinceLastCall,
	float inElapsedTimeSinceLastFlightLoop,
	int inCounter,
	void* inRefcon)
{
	const auto out_data = new_data.get_serialized_data();
	const auto size = new_data.get_serialized_size();

	producer->send_message(out_data, size);

	new_data.reset_builder();
	return -1.0;
}

float retry_callback(float inElapsedSinceLastCall,
	float inElapsedTimeSinceLastFlightLoop,
	int inCounter,
	void* inRefcon)
{
	if (new_data.get_not_found_list_size() != 0) {
		new_data.retry_dataref();
		return 5.0; // Retry after every 5 seconds.
	}
	else {
		// No more uninitialized dataref to process
		return 0.0;
	}
}