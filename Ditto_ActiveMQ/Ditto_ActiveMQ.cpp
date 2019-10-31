#define NOMINMAX

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
void shutdown_activemq();

int GetActiveMQCounter(void* inRefcon);
void SetActiveMQcounter(void* inRefcon, int inValue);

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

	producer->cleanup();
	producer.reset();

	shutdown_activemq();

	XPLMDebugString("Disabling Ditto.\n");
}
PLUGIN_API int  XPluginEnable(void) {
	// First, init the Dataref list when plugin is enabled
	if (new_data.init()) {
		// Initializing ActiveMQ library and Producer
		init_activemq();
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
	catch (const cpptoml::parse_exception & ex)
	{
		XPLMDebugString(ex.what());
		XPLMDebugString("\n");
		return activemq_config{
			"failover:(tcp://192.168.72.249:61616)",
			"XP-Ditto"
		};
	}
}

//Due to ActiveMQ requires that each process has to call ActiveMQCPP::initializeLibrary and ActiveMQCPP::shutdownLibrary,
//when there are multiple plugin that uses ActiveMQ running at the same time, it is required to keep track of which plugin
//should call the init and shutdown function.
//
//Every plugin that use ActiveMQ should participate in a simple bow of keys concept. Each plugin will check whether the "activemq/initialized" dataref exists.
//If the dataref exists then there is already other plugin that was enable before the plugin and the call to ActiveMQCPP::initializeLibrary is already
//performed. So it should not call that function again.
//
//If the dataref does not exist then the plugin is the first one enabled that needs to use ActiveMQ. In that case, the plugin will create the "activemq/initialized" dataref,
//init it to one and then call ActiveMQCPP::initializeLibrary.
//
//During the shutdown process, each of the plugin that use ActiveMQ will decrease the value of "activemq/initialized" dataref by one. If after decrement, the value
//of the dataref returns to zero, that means there is no other plugin that use ActiveMQ anymore. The plugin can then safely call ActiveMQCPP::shutdownLibrary.

void init_activemq()
{
	ActiveMQ = XPLMFindDataRef("activemq/initialized");
	if (ActiveMQ != nullptr) {
		// If the dataref exists then there is another plugin that calls ActiveMQCPP::initializeLibrary already
		// So we should not call it again.
		// We will just increase the dataref by one to inform that we are participating in using ActiveMQ
		auto current = XPLMGetDatai(ActiveMQ);
		XPLMSetDatai(ActiveMQ, (current + 1));
	}
	else {
		// Create the dataref to inform all other plugin that comes later
		ActiveMQ = XPLMRegisterDataAccessor("activemq/initialized",
			xplmType_Int,
			1,
			GetActiveMQCounter, SetActiveMQcounter,
			nullptr, nullptr,
			nullptr, nullptr,
			nullptr, nullptr,
			nullptr, nullptr,
			nullptr, nullptr,
			nullptr, nullptr);

		activemq::library::ActiveMQCPP::initializeLibrary();


		// Find and intialize our counter to one
		ActiveMQ = XPLMFindDataRef("activemq/initialized");
		XPLMSetDatai(ActiveMQ, 1);
	}
}

void shutdown_activemq() {
	// At this point the ActiveMQ should not be null as it was init by either us or other plugin
	// So we just need to decrease it by one and check whether we are the one who should shutdown the library
	auto value = XPLMGetDatai(ActiveMQ);

	if ((value - 1) <= 0) {
		// There is no one else using ActiveMQ we should shutdown here
		activemq::library::ActiveMQCPP::shutdownLibrary();
		XPLMSetDatai(ActiveMQ, value);
	}
	else {
		XPLMSetDatai(ActiveMQ, (value - 1));
	}
}

int GetActiveMQCounter(void* inRefcon)
{
	return activemq_counter_value;
}

void SetActiveMQcounter(void* inRefcon, int inValue)
{
	activemq_counter_value = inValue;
}
