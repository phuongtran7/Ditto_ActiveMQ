#define NOMINMAX

#include "ActiveMQ.h"
#include "Datarefs.h"

#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"

dataref new_data;
XPLMFlightLoopID data_flight_loop_id{};
XPLMFlightLoopID retry_flight_loop_id{};
std::unique_ptr<Producer> producer;

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

PLUGIN_API int XPluginStart(
	char* outName,
	char* outSig,
	char* outDesc)
{
	std::string name = "ActiveMQ Data Extractor";
	std::string signature = "phuong.x-plane.activemq.extractor";
	std::string description = "X-Plane Plugin that send message to an ActiveMQ Broker";

	strcpy_s(outName, name.length() + 1, name.c_str());
	strcpy_s(outSig, signature.length() + 1, signature.c_str());
	strcpy_s(outDesc, description.length() + 1, description.c_str());

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
PLUGIN_API int  XPluginEnable(void) {
	// First, init the Dataref list when plugin is enabled
	if (new_data.init()) {
		// Initializing ActiveMQ library and Producer
		activemq::library::ActiveMQCPP::initializeLibrary();
		auto config = get_activemq_config();
		producer = std::make_unique<Producer>(config.broker_address, config.topic);
		producer->run();

		// If there are some datarefs we cannot find when plugin starts up
		if (new_data.get_not_found_list_size() != 0) {
			// Register a new flight loop to retry finding datarefs
			XPLMCreateFlightLoop_t retry_params{ sizeof(XPLMCreateFlightLoop_t), xplm_FlightLoop_Phase_AfterFlightModel, retry_callback, nullptr };
			retry_flight_loop_id = XPLMCreateFlightLoop(&retry_params);
			if (retry_flight_loop_id != nullptr)
			{
				XPLMScheduleFlightLoop(retry_flight_loop_id, 5, true);
			}
		}

		// Register flight loop for sending data to broker
		XPLMCreateFlightLoop_t data_params{ sizeof(XPLMCreateFlightLoop_t), xplm_FlightLoop_Phase_AfterFlightModel, data_callback, nullptr };
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

	XPLMDebugString("Enabling Ditto.\n");
	return 1;
}
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void* inParam) { }

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

activemq_config get_activemq_config() {
	try
	{
		const auto input_file = cpptoml::parse_file(get_plugin_path() + "Datarefs.toml");
		return activemq_config{
			input_file->get_as<std::string>("address").value_or("failover:(tcp://192.168.72.249:61616)"),
			input_file->get_as<std::string>("topic").value_or("XP-Ditto")
		};
	}
	catch (const cpptoml::parse_exception& ex)
	{
		XPLMDebugString(ex.what());
		XPLMDebugString("\n");
		return activemq_config{
			"failover:(tcp://192.168.72.249:61616)",
			"XP-Ditto"
		};
	}
}