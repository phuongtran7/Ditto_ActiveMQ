#pragma once
#include "Datarefs.h"
class PublishDataref :
	protected dataref
{
private:
	std::unique_ptr<Producer> producer_;
	flexbuffers::Builder flexbuffers_builder_;

private:
	const std::vector<uint8_t>& get_flexbuffers_data();
	size_t get_flexbuffers_size();
	void reset_builder();
	void start_activemq();
	void empty_list() override;

	template<typename T, std::enable_if_t<std::is_same_v<T, int>, int> = 0>
	decltype(auto) get_value(const dataref_info& in_dataref) {
		return XPLMGetDatai(in_dataref.dataref);
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, float>, int> = 0>
	decltype(auto) get_value(const dataref_info& in_dataref) {
		return XPLMGetDataf(in_dataref.dataref);
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, double>, int> = 0>
	decltype(auto) get_value(const dataref_info& in_dataref) {
		return XPLMGetDatad(in_dataref.dataref);
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, std::vector<int>>, int> = 0>
	decltype(auto) get_value(const dataref_info& in_dataref) {
		std::vector<int> temp;
		temp.reserve(in_dataref.num_value.value());
		XPLMGetDatavi(in_dataref.dataref, temp.data(), in_dataref.start_index.value(), in_dataref.num_value.value());
		return temp;
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, std::vector<float>>, int> = 0>
	decltype(auto) get_value(const dataref_info& in_dataref) {
		std::vector<float> temp;
		temp.reserve(in_dataref.num_value.value());
		XPLMGetDatavf(in_dataref.dataref, temp.data(), in_dataref.start_index.value(), in_dataref.num_value.value());
		return temp;
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, std::string>, int> = 0>
	decltype(auto) get_value(const dataref_info& in_dataref) {
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
	PublishDataref(const std::string& topic, const std::string& address, const std::string& config);
	bool init() override;
	void update() override;
	void shutdown() override;
};
