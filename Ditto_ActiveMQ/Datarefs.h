#pragma once
#include <vector>
#include <deque>
#include <optional>
//#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/flexbuffers.h"
//#include "Schema_generated.h"
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

	template<>
	int dataref::get_value(XPLMDataRef in_dataref)
	{
		return XPLMGetDatai(in_dataref);
	}

	template<>
	float dataref::get_value(XPLMDataRef in_dataref)
	{
		return XPLMGetDataf(in_dataref);
	}

	template<>
	double dataref::get_value(XPLMDataRef in_dataref)
	{
		return XPLMGetDatad(in_dataref);
	}

	template <typename V>
	V get_array(XPLMDataRef in_dataref, int start_index, int end_index);

	template<>
	std::vector<int> dataref::get_array(XPLMDataRef in_dataref, int start_index, int number_of_value)
	{
		std::vector<int> temp;
		temp.reserve(number_of_value);
		XPLMGetDatavi(in_dataref, temp.data(), start_index, number_of_value);
		return temp;
	}

	template<>
	std::vector<float> dataref::get_array(XPLMDataRef in_dataref, int start_index, int number_of_value)
	{
		std::vector<float> temp;
		temp.reserve(number_of_value);
		XPLMGetDatavf(in_dataref, temp.data(), start_index, number_of_value);
		return temp;
	}

	template<> 
	std::string dataref::get_array(XPLMDataRef in_dataref, int start_index, int number_of_value)
	{
		// Get the current string size only first
		auto current_string_size = XPLMGetDatab(in_dataref, nullptr, 0, 0);

		// Only get data when there is something in the string dataref
		if (current_string_size != 0) {
			if (start_index == -1) {
				// Get the whole string
				auto temp_buffer_size = current_string_size + 1;
				auto temp = std::make_unique<char[]>(temp_buffer_size);
				XPLMGetDatab(in_dataref, temp.get(), 0, current_string_size);
				return std::string(temp.get());
			}
			else {
				if (number_of_value == -1) {
					// Get the string from start_index to the end
					auto temp_buffer_size = current_string_size + 1;
					auto temp = std::make_unique<char[]>(temp_buffer_size);
					XPLMGetDatab(in_dataref, temp.get(), start_index, current_string_size);
					return std::string(temp.get());
				}
				else {
					// Get part of the string starting from start_index until
					// number_of_value is reached
					auto temp_buffer_size = number_of_value + 1;
					if (number_of_value <= current_string_size) {
						auto temp = std::make_unique<char[]>(temp_buffer_size);
						XPLMGetDatab(in_dataref, temp.get(), start_index, number_of_value);
						return std::string(temp.get());
					}
				}
			}
		}
		return std::string();
	}

	void set_retry_limit();
	flexbuffers::Builder flexbuffers_builder_;
	//flatbuffers::FlatBufferBuilder flatbuffers_builder_;
public:
	std::vector<uint8_t> get_flexbuffers_data();
	size_t get_flexbuffers_size();

	//uint8_t* get_serialized_data();
	//size_t get_serialized_size();
	size_t get_not_found_list_size();
	void retry_dataref();
	void empty_list();
	void reset_builder();
	bool init();
};
