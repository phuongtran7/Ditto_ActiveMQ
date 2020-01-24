#include "Ditto_ActiveMQ.h"

PLUGIN_API int XPluginStart(
	char* outName,
	char* outSig,
	char* outDesc)
{
	std::string name = "ActiveMQ Data Extractor";
	std::string signature = "phuong.x-plane.activemq.extractor";
	std::string description = "X-Plane Plugin that send message to an ActiveMQ Broker";

	strcpy_s(outName, 256, name.c_str());
	strcpy_s(outSig, 256, signature.c_str());
	strcpy_s(outDesc, 256, description.c_str());

	return 1;
}

PLUGIN_API void	XPluginStop(void)
{
	XPLMDebugString("Stopping Ditto.\n");
}

PLUGIN_API void XPluginDisable(void)
{
	// Shutting down each instance of dataref along with the ActiveMQ connection
	for (const auto& item : topic_vector) {
		item->shutdown();
	}

	if (data_flight_loop_id != nullptr) {
		XPLMDestroyFlightLoop(data_flight_loop_id);
	}

	// Shutting down ActiveMQ library
	activemq::library::ActiveMQCPP::shutdownLibrary();

	XPLMDebugString("Disabling Ditto.\n");
}

PLUGIN_API int XPluginEnable(void) {

	// First, look for the config file name in the folder
	auto config = get_config_file_path();

	if (config.empty()) {
		XPLMDebugString("Cannot find configration \".toml\" file. Shutting down.\n");
		return 0;
	}

	// Init ActiveMQ
	activemq::library::ActiveMQCPP::initializeLibrary();

	const auto input_file = cpptoml::parse_file(config);

	// Get the topics
	auto topics = input_file->get_array_of<std::string>("topic");
	auto address = input_file->get_as<std::string>("address").value_or("failover:(tcp://192.168.72.249:61616)");

	for (const auto& topic : *topics)
	{
		auto dataref_instance = std::make_unique<dataref>(topic, address, config);
		if (!dataref_instance->init()) {
			XPLMDebugString(fmt::format("Cannot init topic {}. Shutting down.\n", topic).c_str());
			return 0;
		}
		topic_vector.emplace_back(std::move(dataref_instance));
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

	XPLMDebugString("Enabling Ditto.\n");
	return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void* inParam) {}

std::string get_config_file_path() {
	std::string config_file_path{};
	auto current_path = get_plugin_path();
	for (const auto& entry : std::filesystem::directory_iterator(current_path)) {
		auto ext = entry.path().extension().generic_string();
		if (ext == ".toml") {
			config_file_path = entry.path().generic_string();
		}
	}
	return config_file_path;
}

float data_callback(float inElapsedSinceLastCall,
	float inElapsedTimeSinceLastFlightLoop,
	int inCounter,
	void* inRefcon)
{
	for (const auto& item : topic_vector) {
		item->send_data();
	}
	return -1.0;
}