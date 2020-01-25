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

	if (publish_flight_loop_id != nullptr) {
		XPLMDestroyFlightLoop(publish_flight_loop_id);
	}

	if (subscribe_flight_loop_id != nullptr) {
		XPLMDestroyFlightLoop(subscribe_flight_loop_id);
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
	auto address = input_file->get_as<std::string>("address").value_or("failover:(tcp://192.168.72.249:61616)");

	// Get the publishing topics
	auto publish_topics = input_file->get_array_of<std::string>("publish_topic");
	if (publish_topics) {
		for (const auto& topic : *publish_topics)
		{
			auto dataref_instance = std::make_unique<PublishDataref>(topic, address, config);
			if (!dataref_instance->init()) {
				XPLMDebugString(fmt::format("Cannot init topic {}. Shutting down.\n", topic).c_str());
				return 0;
			}
			topic_vector.emplace_back(std::move(dataref_instance));
		}

		// Register flight loop for sending data to broker
		XPLMCreateFlightLoop_t data_params{ sizeof(XPLMCreateFlightLoop_t), xplm_FlightLoop_Phase_AfterFlightModel, publish_callback, nullptr };
		publish_flight_loop_id = XPLMCreateFlightLoop(&data_params);
		if (publish_flight_loop_id == nullptr)
		{
			XPLMDebugString("Cannot create flight loop. Exiting Ditto.\n");
			return 0;
		}
		else {
			XPLMScheduleFlightLoop(publish_flight_loop_id, -1.0f, true);
		}
	}
	
	// Get the subscribing topic
	auto subscribe_topics = input_file->get_array_of<std::string>("subscribe_topic");
	if (subscribe_topics) {

	}


	XPLMDebugString("Enabling Ditto.\n");
	return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void* inParam) {}

std::string get_config_file_path() {
	auto current_path = get_plugin_path();
	for (const auto& entry : std::filesystem::directory_iterator(current_path)) {
		// Look for ".toml" file in the plugin directory
		auto ext = entry.path().extension().generic_string();
		if (ext == ".toml") {
			return entry.path().generic_string();
		}
	}
	return std::string{};
}

float publish_callback(float inElapsedSinceLastCall,
	float inElapsedTimeSinceLastFlightLoop,
	int inCounter,
	void* inRefcon)
{
	for (const auto& item : topic_vector) {
		item->update();
	}
	return -1.0;
}