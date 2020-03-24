// This class is the base class for PublishTopic and SubscribeTopic

#pragma once
#include "MQTT_Client.h"
#include <vector>
#include <deque>
#include <optional>
#include "flatbuffers/flexbuffers.h"
#include "Utility.h"
#include <mutex>
#include <fmt/format.h>
#include <type_traits>

class Topic
{
protected:
	enum class DatarefType {
		STRING,
		INT,
		FLOAT,
		DOUBLE
	};

	struct DatarefInfo {
		std::string dataref_name{}; // Name if dataref, i.e "sim/cockpit/" something
		std::string name{}; // Name user defined for the dataref
		XPLMDataRef dataref{};
		DatarefType type{};
		std::optional<int> start_index{};
		std::optional<int> num_value{}; // Number of values in the array to get; starts at start_index
	};

	std::string topic_;
	std::string address_;
	std::string config_file_path_;
	std::mutex data_lock;
	std::vector<DatarefInfo> dataref_list_;
	std::vector<DatarefInfo> not_found_list_;
	flexbuffers::Builder flexbuffers_builder_;
	int retry_limit{};
	int retry_num{};

protected:
	bool get_data_list();
	void set_retry_limit();
	size_t get_not_found_list_size();
	void retry_dataref();
	virtual void empty_list();

public:
	explicit Topic(const std::string& topic, const std::string& address, const std::string& config);
	virtual ~Topic() = default;
	virtual bool init();
	virtual void update() = 0;
	virtual void shutdown();
};