#include "PublishDataref.h"

void PublishDataref::start_activemq()
{
	producer_ = std::make_unique<Producer>(address_, topic_);
	producer_->run();
}

void PublishDataref::empty_list()
{
	// Try and get access to dataref_list_
	std::scoped_lock<std::mutex> guard(data_lock);
	dataref_list_.clear();
	not_found_list_.clear();
	reset_builder();
}

PublishDataref::PublishDataref(const std::string& topic, const std::string& address, const std::string& config) :
	dataref(topic, address, config)
{
}

bool PublishDataref::init()
{
	if (get_data_list()) {
		start_activemq();
		set_retry_limit();
		return true;
	}
	return false;
}

void PublishDataref::update()
{
	const auto out_data = get_flexbuffers_data();
	const auto size = get_flexbuffers_size();
	producer_->send_message(out_data, size);
	reset_builder();
}

void PublishDataref::shutdown()
{
	empty_list();
	producer_->cleanup();
	producer_.reset();
}

const std::vector<uint8_t>& PublishDataref::get_flexbuffers_data() {
	// Try and get access to dataref_list_
	std::scoped_lock<std::mutex> guard(data_lock);

	const auto map_start = flexbuffers_builder_.StartMap();

	for (const auto& dataref : dataref_list_) {
		switch (dataref.type) {
		case DatarefType::INT: {
			if (dataref.start_index.has_value()) {
				// If start index exist then it's an array
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
			else {
				// Just single value
				auto return_value = get_value<int>(dataref);
				flexbuffers_builder_.Int(dataref.name.c_str(), return_value);
			}
			break;
		}
		case DatarefType::FLOAT: {
			if (dataref.start_index.has_value()) {
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
			else {
				flexbuffers_builder_.Float(dataref.name.c_str(), get_value<float>(dataref));
			}
			break;
		}
		case DatarefType::DOUBLE: {
			flexbuffers_builder_.Double(dataref.name.c_str(), get_value<double>(dataref));
			break;
		}
		case DatarefType::STRING: {
			flexbuffers_builder_.String(dataref.name.c_str(), get_value<std::string>(dataref).c_str());
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

size_t PublishDataref::get_flexbuffers_size() {
	return flexbuffers_builder_.GetSize();
}

void PublishDataref::reset_builder() {
	flexbuffers_builder_.Clear();
}