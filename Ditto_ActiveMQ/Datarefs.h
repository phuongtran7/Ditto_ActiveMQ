#pragma once

#include "ActiveMQ.h"
#include <vector>
#include <deque>
#include <optional>
#include "flatbuffers/flexbuffers.h"
#include "Utility.h"
#include <mutex>
#include <fmt/format.h>
#include <type_traits>

class dataref {
protected:
	enum class DatarefType {
		STRING,
		INT,
		FLOAT,
		DOUBLE
	};

	struct dataref_info {
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
	std::vector<dataref_info> dataref_list_;
	std::vector<dataref_info> not_found_list_;
	int retry_limit{};
	int retry_num{};

protected:
	bool get_data_list();
	void set_retry_limit();
	void retry_dataref();
	virtual void empty_list();
	size_t get_not_found_list_size();

public:
	explicit dataref(const std::string& topic, const std::string& address, const std::string& config);
	virtual ~dataref() = default;
	virtual bool init();
	virtual void update();
	virtual void shutdown();
};