#include "Topic.h"

size_t Topic::get_not_found_list_size() {
	return not_found_list_.size();
}

// Remove all the dataref in the dataref list
void Topic::empty_list() {
	// Try and get access to dataref_list_
	std::scoped_lock guard(data_lock);
	dataref_list_.clear();
	not_found_list_.clear();
}

Topic::Topic(std::string topic, std::string address, std::string config) :
	topic_(std::move(topic)),
	address_(std::move(address)),
	config_file_path_(std::move(config)),
	dataref_list_{},
	not_found_list_{}
{
}

void Topic::set_retry_limit() {
	try {
		const auto input_file = cpptoml::parse_file(config_file_path_);
		retry_limit = input_file->get_as<int>("retry_limit").value_or(0);
		retry_num = 1;
	}
	catch (const cpptoml::parse_exception & ex) {
		XPLMDebugString(ex.what());
		XPLMDebugString("\n");
	}
}

void Topic::retry_dataref() {
	// Try and get access to not_found_list_ and dataref_list_
	std::scoped_lock guard(data_lock);

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

bool Topic::get_data_list() {
	try {
		const auto input_file = cpptoml::parse_file(config_file_path_);
		// Get the list of all the dataref that this instance is handling
		const auto data_list = input_file->get_table_array(topic_);
		if (data_list != nullptr) {
			// Loop through all the tables
			for (const auto& table : *data_list) {
				auto temp_name = table->get_as<std::string>("string").value_or("");

				XPLMDataRef new_dataref = XPLMFindDataRef(temp_name.c_str());

				auto start = table->get_as<int>("start_index").value_or(-1);
				auto num = table->get_as<int>("num_value").value_or(-1);
				DatarefInfo temp_dataref_info;

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

				if (temp_dataref_info.dataref == nullptr) {
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

bool Topic::init()
{
	if (get_data_list()) {
		set_retry_limit();
		return true;
	}
	return false;
}

void Topic::update()
{
}

void Topic::shutdown()
{
	empty_list();
}