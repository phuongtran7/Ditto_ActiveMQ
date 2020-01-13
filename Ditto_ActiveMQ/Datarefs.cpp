#include "Datarefs.h"

std::vector<dataref::dataref_info> dataref::get_list() { return dataref_list_; }

size_t dataref::get_not_found_list_size() { return not_found_list_.size(); }

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

std::vector<uint8_t> dataref::get_flexbuffers_data() {
	// Try and get access to dataref_list_
	std::scoped_lock<std::mutex> guard(data_lock);

	const auto map_start = flexbuffers_builder_.StartMap();

	for (const auto& dataref : dataref_list_) {
		// String is special case so handle it first
		if (dataref.type == "string") {
			auto str = get_value<std::string>(dataref);
			if (!str.empty()) {
				flexbuffers_builder_.String(dataref.name.c_str(), str.c_str());
			}
		}
		else {
			// If start and end index does not present that means the dataref is
			// single value dataref
			if (!dataref.start_index.has_value() && !dataref.num_value.has_value()) {
				if (dataref.type == "int") {
					flexbuffers_builder_.Int(dataref.name.c_str(), get_value<int>(dataref));
				}
				else if (dataref.type == "float") {
					flexbuffers_builder_.Float(dataref.name.c_str(), get_value<float>(dataref));
				}
				else if (dataref.type == "double") {
					flexbuffers_builder_.Double(dataref.name.c_str(), get_value<double>(dataref));
				}
			}
			else {
				if (dataref.type == "int") {
					auto int_num = get_value<std::vector<int>>(dataref);
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
				}
				else if (dataref.type == "float") {
					auto float_num = get_value<std::vector<float>>(dataref);
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
		const auto input_file = cpptoml::parse_file(get_plugin_path() + "Datarefs.toml");
		retry_limit = input_file->get_as<int>("retry_limit").value_or(0);
		retry_num = 1;
	}
	catch (const cpptoml::parse_exception & ex) {
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
				temp_dataref_info.type = table->get_as<std::string>("type").value_or("");

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
