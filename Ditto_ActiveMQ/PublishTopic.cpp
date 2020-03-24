#include "PublishTopic.h"

void PublishTopic::prepare_flexbuffers_data()
{
	// Try and get access to dataref_list_
	std::scoped_lock guard(data_lock);

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
						for (auto i : int_num) {
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
						for (auto i : float_num) {
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

	//return flexbuffers_builder_.GetBuffer();
}

size_t PublishTopic::get_flexbuffers_size()
{
	return flexbuffers_builder_.GetSize();
}

void PublishTopic::reset_builder()
{
	flexbuffers_builder_.Clear();
}

void PublishTopic::start_publisher()
{
	//publisher_ = std::make_unique<MQTT_Publisher>(address_, topic_, 0);
	publisher = std::make_unique<MQTT_Client>(address_, topic_, 0);
}

void PublishTopic::empty_list()
{
	// Try and get access to dataref_list_
	std::scoped_lock guard(data_lock);
	dataref_list_.clear();
	not_found_list_.clear();
	reset_builder();
}

PublishTopic::PublishTopic(const std::string& topic, const std::string& address, const std::string& config) :
	Topic(topic, address, config)
{
}

bool PublishTopic::init()
{
	if (get_data_list()) {
		start_publisher();
		set_retry_limit();
		return true;
	}
	return false;
}

void PublishTopic::update()
{
	prepare_flexbuffers_data();
	//publisher_->send_message(flexbuffers_builder_.GetBuffer(), flexbuffers_builder_.GetSize());
	publisher->send_message(flexbuffers_builder_.GetBuffer());
	reset_builder();
}

void PublishTopic::shutdown()
{
	empty_list();
	//publisher_.reset();
	publisher.reset();
}
