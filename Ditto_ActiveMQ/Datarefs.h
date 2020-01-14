#pragma once

#include <vector>
#include <deque>
#include <optional>
#include "flatbuffers/flexbuffers.h"
#include "Utility.h"
#include <mutex>
#include <fmt/format.h>
#include <variant>

using DataType = std::variant<int, float, double, std::string, std::vector<int>, std::vector<float>>;

class dataref {
private:
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

	std::mutex data_lock;
	std::vector<dataref_info> dataref_list_;
	std::vector<dataref_info> not_found_list_;
	flexbuffers::Builder flexbuffers_builder_;
	int retry_limit{};
	int retry_num{};

private:
	bool get_data_list();
	std::vector<dataref_info>& get_list();
	void set_retry_limit();
	DataType get_dataref_value(const dataref_info& in_dataref);

public:
	const std::vector<uint8_t>& get_flexbuffers_data();
	size_t get_flexbuffers_size();
	size_t get_not_found_list_size();
	void retry_dataref();
	void empty_list();
	void reset_builder();
	bool init();
};
