// This class is the base class for PublishTopic and SubscribeTopic

#pragma once
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
public:
	enum class TopicRole {
		PUBLISHER,
		SUBSCRIBER
	};

private:
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

	TopicRole role_;
	std::string topic_;
	std::string address_;
	std::string config_file_path_;
	std::mutex data_lock;
	std::vector<DatarefInfo> dataref_list_;
	std::vector<DatarefInfo> not_found_list_;
	flexbuffers::Builder flexbuffers_builder_;
	int retry_limit{};
	int retry_num{};

private:
	bool get_data_list();
	void set_retry_limit();
	const std::vector<uint8_t>& get_flexbuffers_data();
	size_t get_flexbuffers_size();
	size_t get_not_found_list_size();
	void retry_dataref();
	void empty_list();
	void reset_builder();

	template<typename T, std::enable_if_t<std::is_same_v<T, int>, int> = 0>
	decltype(auto) get_value(const DatarefInfo& in_dataref) {
		return XPLMGetDatai(in_dataref.dataref);
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, float>, int> = 0>
	decltype(auto) get_value(const DatarefInfo& in_dataref) {
		return XPLMGetDataf(in_dataref.dataref);
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, double>, int> = 0>
	decltype(auto) get_value(const DatarefInfo& in_dataref) {
		return XPLMGetDatad(in_dataref.dataref);
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, std::vector<int>>, int> = 0>
	decltype(auto) get_value(const DatarefInfo& in_dataref) {
		std::vector<int> temp{};
		temp.resize(in_dataref.num_value.value());
		XPLMGetDatavi(in_dataref.dataref, temp.data(), in_dataref.start_index.value(), in_dataref.num_value.value());
		return temp;
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, std::vector<float>>, int> = 0>
	decltype(auto) get_value(const DatarefInfo& in_dataref) {
		std::vector<float> temp{};
		temp.resize(in_dataref.num_value.value());
		XPLMGetDatavf(in_dataref.dataref, temp.data(), in_dataref.start_index.value(), in_dataref.num_value.value());
		return temp;
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, std::string>, int> = 0>
	decltype(auto) get_value(const DatarefInfo& in_dataref) {
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
	explicit Topic(TopicRole role, const std::string& topic, const std::string& address, const std::string& config);
	bool init();
	void update();
	void shutdown();
};

