#pragma once
#include <vector>
#include <deque>
#include <optional>
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/flexbuffers.h"
#include "Schema_generated.h"
#include "Utility.h"
#include <mutex>

class dataref {
private:
	struct dataref_info {
		std::string dataref_name{}; // Name if dataref, i.e "sim/cockpit/" something
		std::string name{}; // Name user defined for the dataref
		XPLMDataRef dataref{};
		std::string type{};
		std::optional<int> start_index{};
		std::optional<int> num_value{}; // Number of values in the array to get; starts at start_index
	};
	std::mutex data_lock;
	std::vector<dataref_info> dataref_list_;
	std::vector<dataref_info> not_found_list_;
	std::vector<dataref_info> get_list();
	bool get_data_list();
	int retry_limit{};
	int retry_num{};

	template <typename T>
	T get_value(XPLMDataRef in_dataref);

	template <typename V>
	V get_array(XPLMDataRef in_dataref, int start_index, int end_index);

	std::vector<uint8_t> get_flexbuffers_data();
	size_t get_flexbuffers_size();
	void set_retry_limit();
	flexbuffers::Builder flexbuffers_builder_;
	flatbuffers::FlatBufferBuilder flatbuffers_builder_;
public:
	uint8_t* get_serialized_data();
	size_t get_serialized_size();
	size_t get_not_found_list_size();
	void retry_dataref();
	void empty_list();
	void reset_builder();
	bool init();
};
