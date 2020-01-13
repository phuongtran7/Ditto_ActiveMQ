#pragma once

#include <vector>
#include <deque>
#include <optional>
#include "flatbuffers/flexbuffers.h"
#include "Utility.h"
#include <mutex>
#include <fmt/format.h>

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

	template <typename T>
	T get_value(const dataref_info& in_dataref);

	template<>
	int get_value(const dataref_info& in_dataref)
	{
		return XPLMGetDatai(in_dataref.dataref);
	}

	template<>
	float get_value(const dataref_info& in_dataref)
	{
		return XPLMGetDataf(in_dataref.dataref);
	}

	template<>
	double get_value(const dataref_info& in_dataref)
	{
		return XPLMGetDatad(in_dataref.dataref);
	}

	template<>
	std::vector<int> get_value(const dataref_info& in_dataref)
	{
		std::vector<int> temp;
		temp.reserve(in_dataref.num_value.value());
		XPLMGetDatavi(in_dataref.dataref, temp.data(), in_dataref.start_index.value(), in_dataref.num_value.value());
		return temp;
	}

	template<>
	std::vector<float> get_value(const dataref_info& in_dataref)
	{
		std::vector<float> temp;
		temp.reserve(in_dataref.num_value.value());
		XPLMGetDatavf(in_dataref.dataref, temp.data(), in_dataref.start_index.value(), in_dataref.num_value.value());
		return temp;
	}

	template<> 
	std::string get_value(const dataref_info& in_dataref)
	{
		// Get the current string size only first
		auto current_string_size = XPLMGetDatab(in_dataref.dataref, nullptr, 0, 0);

		// Only get data when there is something in the string dataref
		if (current_string_size != 0) {
			if (!in_dataref.start_index.has_value()) {
				// Get the whole string
				auto temp_buffer_size = current_string_size + 1;
				auto temp = std::make_unique<char[]>(temp_buffer_size);
				XPLMGetDatab(in_dataref.dataref, temp.get(), 0, current_string_size);
				return std::string(temp.get());
			}
			else {
				if (!in_dataref.num_value.has_value()) {
					// Get the string from start_index to the end
					auto temp_buffer_size = current_string_size + 1;
					auto temp = std::make_unique<char[]>(temp_buffer_size);
					XPLMGetDatab(in_dataref.dataref, temp.get(), in_dataref.start_index.value(), current_string_size);
					return std::string(temp.get());
				}
				else {
					// Get part of the string starting from start_index until
					// number_of_value is reached
					auto temp_buffer_size = in_dataref.num_value.value() + 1;
					if (in_dataref.num_value.value() <= current_string_size) {
						auto temp = std::make_unique<char[]>(temp_buffer_size);
						XPLMGetDatab(in_dataref.dataref, temp.get(), in_dataref.start_index.value(), in_dataref.num_value.value());
						return std::string(temp.get());
					}
				}
			}
		}
		return std::string();
	}

public:
	const std::vector<uint8_t>& get_flexbuffers_data();
	size_t get_flexbuffers_size();
	size_t get_not_found_list_size();
	void retry_dataref();
	void empty_list();
	void reset_builder();
	bool init();
};
