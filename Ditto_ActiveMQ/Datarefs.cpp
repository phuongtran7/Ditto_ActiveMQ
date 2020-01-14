#include "Datarefs.h"

using DataType = std::variant<int, float, double, std::string, std::vector<int>, std::vector<float>>;

size_t dataref::get_not_found_list_size() {
	return not_found_list_.size();
}

void dataref::reset_builder() {
	flexbuffers_builder_.Clear();
}

// Remove all the dataref in the dataref list
void dataref::empty_list() {
	// Try and get access to dataref_list_
	std::scoped_lock<std::mutex> guard(data_lock);
	dataref_list_.clear();
	not_found_list_.clear();
	reset_builder();
}

const std::vector<uint8_t>& dataref::get_flexbuffers_data() {
	// Try and get access to dataref_list_
	std::scoped_lock<std::mutex> guard(data_lock);

	const auto map_start = flexbuffers_builder_.StartMap();

	for (const auto& dataref : dataref_list_) {

		auto return_value = get_dataref_value(dataref);

		// Add the return value to approriate flexbuffers type
		auto index = return_value.index();
		switch (index) {
			// std::variant<int, float, double, std::string, std::vector<int>, std::vector<float>>
		case 0: {
			// Holding a int value
			flexbuffers_builder_.Int(dataref.name.c_str(), std::get<int>(return_value));
			break;
		}
		case 1: {
			// Holding a float value
			flexbuffers_builder_.Float(dataref.name.c_str(), std::get<float>(return_value));
			break;
		}
		case 2: {
			// Holding a double value
			flexbuffers_builder_.Double(dataref.name.c_str(), std::get<double>(return_value));
			break;
		}
		case 3: {
			// Holding a string
			flexbuffers_builder_.String(dataref.name.c_str(), std::get<std::string>(return_value).c_str());
			break;
		}
		case 4: {
			// Holding a vector of int
			auto int_num = std::get<std::vector<int>>(return_value);
			if (2 <= dataref.num_value.value() && dataref.num_value.value() <= 4) {
				flexbuffers_builder_.FixedTypedVector(dataref.name.c_str(),
					int_num.data(),
					dataref.num_value.value());
			}
			else {
				flexbuffers_builder_.TypedVector(dataref.name.c_str(), [&] {
					for (auto& i : int_num) {
						flexbuffers_builder_.Int(i);
					}
					});
			}
			break;
		}
		case 5: {
			// Holding a vector of float
			auto float_num = std::get<std::vector<float>>(return_value);
			if (2 <= dataref.num_value.value() && dataref.num_value.value() <= 4) {
				flexbuffers_builder_.FixedTypedVector(dataref.name.c_str(),
					float_num.data(),
					dataref.num_value.value());
			}
			else {
				flexbuffers_builder_.TypedVector(dataref.name.c_str(), [&] {
					for (auto& i : float_num) {
						flexbuffers_builder_.Float(i);
					}
					});
			}
			break;
		}
		default:
			break;
		}
	}

	flexbuffers_builder_.EndMap(map_start);
	flexbuffers_builder_.Finish();

	return flexbuffers_builder_.GetBuffer();
}

size_t dataref::get_flexbuffers_size() {
	return flexbuffers_builder_.GetSize();
}

void dataref::set_retry_limit() {
	try {
		const auto input_file = cpptoml::parse_file(get_plugin_path() + "Datarefs.toml");
		retry_limit = input_file->get_as<int>("retry_limit").value_or(0);
		retry_num = 1;
	}
	catch (const cpptoml::parse_exception & ex) {
		XPLMDebugString(ex.what());
		XPLMDebugString("\n");
	}
}

// Get the data depending on what type of the dataref is
// Return a std::variant<int, float, double, std::string, std::vector<int>, std::vector<float>>
DataType dataref::get_dataref_value(const dataref_info& in_dataref)
{
	// Handle whether the dataref is a string or not first
	if (in_dataref.type == DatarefType::STRING) {
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
	else {
		switch (in_dataref.type)
		{
		case DatarefType::INT: {
			// If start and end index present then it's an array
			if (in_dataref.start_index.has_value()) {
				std::vector<int> temp;
				temp.reserve(in_dataref.num_value.value());
				XPLMGetDatavi(in_dataref.dataref, temp.data(), in_dataref.start_index.value(), in_dataref.num_value.value());
				return temp;
			}
			// Else return single value
			return XPLMGetDatai(in_dataref.dataref);
		}
		case DatarefType::FLOAT: {
			// If start and end index present then it's an array
			if (in_dataref.start_index.has_value()) {
				std::vector<float> temp;
				temp.reserve(in_dataref.num_value.value());
				XPLMGetDatavf(in_dataref.dataref, temp.data(), in_dataref.start_index.value(), in_dataref.num_value.value());
				return temp;
			}
			// Else return single value
			return XPLMGetDataf(in_dataref.dataref);
		}
		case DatarefType::DOUBLE: {
			// Double only has single value in X-Plane
			return XPLMGetDatad(in_dataref.dataref);
		}
		default:
			return DataType{};
		}
	}
	return DataType{};
}

void dataref::retry_dataref() {
	// Try and get access to not_found_list_ and dataref_list_
	std::scoped_lock<std::mutex> guard(data_lock);

	// TO DO: add a flag in Dataref.toml to mark a dataref that will be created by
	// another plugin later so that Ditto can search for it later after the plane
	// loaded. XPLMFindDataRef is rather expensive so avoid using this
	if (!not_found_list_.empty() && retry_num <= retry_limit) {
		XPLMDebugString(fmt::format("Cannot find {} dataref. Retrying.\n", not_found_list_.size()).c_str());
		for (auto it = not_found_list_.begin(); it != not_found_list_.end();) {
			XPLMDebugString(fmt::format("Retrying {}.\n", it->dataref_name).c_str());
			it->dataref = XPLMFindDataRef(it->dataref_name.c_str());
			if (it->dataref != nullptr) {
				// Add the newly found dataref to dataref_list_
				dataref_list_.emplace_back(std::move(*it));
				// Remove from the not found list
				it = not_found_list_.erase(it);
			}
			else {
				++it;
			}
		}
		retry_num++;
	}
	else {
		// Empty the not_found_list_ to un-register the callback for retrying
		not_found_list_.clear();
	}
}

bool dataref::get_data_list() {
	try {
		const auto input_file =
			cpptoml::parse_file(get_plugin_path() + "Datarefs.toml");
		// Create a list of all the Data table in the toml file
		const auto data_list = input_file->get_table_array("Data");

		if (data_list != nullptr) {
			// Loop through all the tables
			for (const auto& table : *data_list) {
				auto temp_name = table->get_as<std::string>("string").value_or("");

				XPLMDataRef new_dataref = XPLMFindDataRef(temp_name.c_str());

				auto start = table->get_as<int>("start_index").value_or(-1);
				auto num = table->get_as<int>("num_value").value_or(-1);
				dataref_info temp_dataref_info;

				temp_dataref_info.dataref_name = temp_name;
				temp_dataref_info.name = table->get_as<std::string>("name").value_or("");
				temp_dataref_info.dataref = new_dataref;

				auto temp_type = table->get_as<std::string>("type").value_or("");
				if (temp_type == "int") {
					temp_dataref_info.type = DatarefType::INT;
				}
				else if (temp_type == "float") {
					temp_dataref_info.type = DatarefType::FLOAT;
				}
				else if (temp_type == "double") {
					temp_dataref_info.type = DatarefType::DOUBLE;
				}
				else if (temp_type == "string") {
					temp_dataref_info.type = DatarefType::STRING;
				}
				
				if (start != -1) {
					temp_dataref_info.start_index = start;
				}
				else {
					temp_dataref_info.start_index = std::nullopt;
				}

				if (num != -1) {
					temp_dataref_info.num_value = num;
				}
				else {
					temp_dataref_info.num_value = std::nullopt;
				}

				if (temp_dataref_info.dataref == NULL) {
					// Push to not found list to retry at later time
					not_found_list_.emplace_back(temp_dataref_info);
				}
				else {
					dataref_list_.emplace_back(temp_dataref_info);
				}
			}
			// Table empty
			return true;
		}
		return false;
	}
	catch (const cpptoml::parse_exception & ex) {
		XPLMDebugString(ex.what());
		XPLMDebugString("\n");
		return false;
	}
}

bool dataref::init() {
	if (get_data_list()) {
		set_retry_limit();
		return true;
	}
	return false;
}
