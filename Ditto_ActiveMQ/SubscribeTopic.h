#pragma once
#include "Topic.h"

class SubscribeTopic :
	protected Topic
{
private:
	std::shared_ptr<synchronized_value<std::string>> buffer_;
	std::unique_ptr<MQTT_Client> subscriber_;

private:
	void empty_list() override;
	void start_subscriber();

	template<typename T, std::enable_if_t<std::is_same_v<T, int>, int> = 0>
	void set_value(const DatarefInfo& in_dataref, T input) {
		XPLMSetDatai(in_dataref.dataref, input);
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, float>, int> = 0>
	void set_value(const DatarefInfo& in_dataref, T input) {
		XPLMSetDataf(in_dataref.dataref, input);
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, double>, int> = 0>
	void set_value(const DatarefInfo& in_dataref, T input) {
		XPLMSetDatad(in_dataref.dataref, input);
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, std::vector<int>>, int> = 0>
	void set_value(const DatarefInfo& in_dataref, T input) {
		XPLMSetDatavi(in_dataref.dataref, const_cast<int*>(&input[0]), in_dataref.start_index.value(), in_dataref.num_value.value());
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, std::vector<float>>, int> = 0>
	void set_value(const DatarefInfo& in_dataref, T input) {
		XPLMSetDatavf(in_dataref.dataref, const_cast<float*>(&input[0]), in_dataref.start_index.value(), in_dataref.num_value.value());
	}

public:
	SubscribeTopic(std::string topic, std::string address, std::string config);
	bool init() override;
	void update() override;
	void shutdown() override;
};

