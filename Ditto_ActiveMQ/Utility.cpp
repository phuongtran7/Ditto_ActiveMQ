#include "Utility.h"

std::string get_plugin_path()
{
	const auto my_id = XPLMFindPluginBySignature("phuong.x-plane.activemq.extractor");
	std::vector<char> buffer(1024);
	XPLMGetPluginInfo(my_id, nullptr, buffer.data(), nullptr, nullptr);
	auto return_string = std::string(buffer.data());
	auto pos = return_string.find(R"(64\win.xpl)");
	if (pos != std::string::npos)
	{
		// XPLMGetPluginInfo return absolute path to win.xpl file so it need to be trimmed off the string
		return_string.erase(pos, return_string.length()); // Trim "64\win.xpl"
	}
	return return_string;
}

aircraft_info get_loaded_aircraft()
{
	std::vector<char> name_buffer(256);
	std::vector<char> path_buffer(1024);
	XPLMGetNthAircraftModel(XPLM_USER_AIRCRAFT, name_buffer.data(), path_buffer.data());
	return aircraft_info{ name_buffer.data(), path_buffer.data() };
}

std::string get_config_path() {
	auto loaded_aircraft = get_loaded_aircraft();
	auto pos = loaded_aircraft.aircraft_path.find(loaded_aircraft.aircraft_name);
	if (pos != std::string::npos)
	{
		loaded_aircraft.aircraft_path.erase(pos, loaded_aircraft.aircraft_name.length());
	}
	return loaded_aircraft.aircraft_path;
}