#include "Datarefs.h"

std::vector<dataref::dataref_info> dataref::get_list() { return dataref_list_; }

size_t dataref::get_not_found_list_size() { return not_found_list_.size(); }

void dataref::reset_builder() {
	flexbuffers_builder_.Clear();
	flatbuffers_builder_.Clear();
}

// Remove all the dataref in the dataref list
void dataref::empty_list() {
	// Try and get access to dataref_list_
	std::scoped_lock<std::mutex> guard(data_lock);
	dataref_list_.clear();
	not_found_list_.clear();
	reset_builder();
}

uint8_t* dataref::get_serialized_data() {
	const auto data = flatbuffers_builder_.CreateVector(get_flexbuffers_data());
	const auto size = get_flexbuffers_size();

	Ditto::DataBuilder data_builder(flatbuffers_builder_);
	data_builder.add_size(size);
	data_builder.add_buffer(data);
	const auto finished = data_builder.Finish();

	flatbuffers_builder_.Finish(finished);

	return flatbuffers_builder_.GetBufferPointer();
}

size_t dataref::get_serialized_size() { return flatbuffers_builder_.GetSize(); }

std::vector<uint8_t> dataref::get_flexbuffers_data() {
	// Try and get access to dataref_list_
	std::scoped_lock<std::mutex> guard(data_lock);

	const auto map_start = flexbuffers_builder_.StartMap();

	for (auto& dataref : dataref_list_) {
		// String is special case so handle it first
		if (dataref.type == "string") {
			auto str =
				get_value_string(dataref.dataref, dataref.start_index.value_or(-1),
					dataref.num_value.value_or(-1));
			if (!str.empty()) {
				flexbuffers_builder_.String(dataref.name.c_str(), str.c_str());
			}
		}
		else {
			// If start and end index does not present that means the dataref is
			// single value dataref
			if (!dataref.start_index.has_value() && !dataref.num_value.has_value()) {
				if (dataref.type == "int") {
					flexbuffers_builder_.Int(dataref.name.c_str(),
						get_value_int(dataref.dataref));
				}
				else if (dataref.type == "float") {
					flexbuffers_builder_.Float(dataref.name.c_str(),
						get_value_float(dataref.dataref));
				}
				else if (dataref.type == "double") {
					flexbuffers_builder_.Double(dataref.name.c_str(),
						get_value_double(dataref.dataref));
				}
			}
			else {
				if (dataref.type == "int") {
					auto int_num =
						get_value_int_array(dataref.dataref, dataref.start_index.value(),
							dataref.num_value.value());
					if (2 <= dataref.num_value.value() &&
						dataref.num_value.value() <= 4) {
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
				}
				else if (dataref.type == "float") {
					auto float_num = get_value_float_array(dataref.dataref,
						dataref.start_index.value(),
						dataref.num_value.value());
					if (2 <= dataref.num_value.value() &&
						dataref.num_value.value() <= 4) {
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
				}
			}
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
		const auto input_file =
			cpptoml::parse_file(get_plugin_path() + "Datarefs.toml");
		retry_limit = input_file->get_as<int>("retry_limit").value_or(0);
		retry_num = 1;
	}
	catch (const cpptoml::parse_exception& ex) {
		XPLMDebugString(ex.what());
		XPLMDebugString("\n");
	}
}

void dataref::retry_dataref() {
	// Try and get access to not_found_list_ and dataref_list_
	std::scoped_lock<std::mutex> guard(data_lock);

	// TO DO: add a flag in Dataref.toml to mark a dataref that will be created by
	// another plugin later so that Ditto can search for it later after the plane
	// loaded. XPLMFindDataRef is rather expensive so avoid using this
	if (!not_found_list_.empty() && retry_num <= retry_limit) {
		XPLMDebugString(("Cannot find " + std::to_string(not_found_list_.size()) +
			" dataref. Retrying.\n")
			.c_str());
		for (auto it = not_found_list_.begin(); it != not_found_list_.end(); ++it) {
			std::string s = "Retrying " + it->dataref_name + "\n";
			XPLMDebugString(s.c_str());
			it->dataref = XPLMFindDataRef(it->dataref_name.c_str());
			if (it->dataref != nullptr) {
				// Remove the newly found dataref from the not found list
				not_found_list_.erase(it);
				// Add it to dataref_list_
				dataref_list_.emplace_back(*it);
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
				temp_dataref_info.name =
					table->get_as<std::string>("name").value_or("");
				temp_dataref_info.dataref = new_dataref;
				temp_dataref_info.type =
					table->get_as<std::string>("type").value_or("");

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
	catch (const cpptoml::parse_exception& ex) {
		XPLMDebugString(ex.what());
		XPLMDebugString("\n");
		return false;
	}
}

int dataref::get_value_int(XPLMDataRef in_dataref) {
	return XPLMGetDatai(in_dataref);
}

float dataref::get_value_float(XPLMDataRef in_dataref) {
	return XPLMGetDataf(in_dataref);
}

double dataref::get_value_double(XPLMDataRef in_dataref) {
	return XPLMGetDatad(in_dataref);
}

std::vector<int> dataref::get_value_int_array(XPLMDataRef in_dataref,
	int start_index,
	int number_of_value) {
	std::vector<int> temp;
	temp.reserve(number_of_value);
	XPLMGetDatavi(in_dataref, temp.data(), start_index, number_of_value);
	return temp;
}

std::vector<float> dataref::get_value_float_array(XPLMDataRef in_dataref,
	int start_index,
	int number_of_value) {
	std::vector<float> temp;
	temp.reserve(number_of_value);
	XPLMGetDatavf(in_dataref, temp.data(), start_index, number_of_value);
	return temp;
}

std::string dataref::get_value_string(XPLMDataRef in_dataref, int start_index,
	int number_of_value) {
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

bool dataref::init() {
	if (get_data_list()) {
		set_retry_limit();
		return true;
	}
	return false;
}
