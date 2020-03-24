#include "SubscribeTopic.h"

void SubscribeTopic::empty_list()
{
	// Try and get access to dataref_list_
	std::scoped_lock guard(data_lock);
	dataref_list_.clear();
	not_found_list_.clear();
}

void SubscribeTopic::start_subscriber()
{
	subscriber_ = std::make_unique<MQTT_Client>(address_, topic_, 0, buffer_);
}

SubscribeTopic::SubscribeTopic(const std::string& topic, const std::string& address, const std::string& config) :
	Topic(topic, address, config)
{
}

bool SubscribeTopic::init()
{
	if (get_data_list()) {
		start_subscriber();
		return true;
	}
	return false;
}

void SubscribeTopic::update()
{
	// Try and get access to dataref_list_
	std::scoped_lock guard(data_lock);

	auto received_data = apply([](std::string& s) { return std::move(s); }, *buffer_);

	if (!received_data.empty()) {

		auto data = flexbuffers::GetRoot(reinterpret_cast<const uint8_t*>(received_data.data()), received_data.size()).AsMap();

		for (const auto& dataref : dataref_list_)
		{
			switch (dataref.type) {
			case DatarefType::INT: {
				if (dataref.start_index.has_value()) {
					// If start index exist then it's an array
					auto temp = data[dataref.name].AsFixedTypedVector();
					std::vector<int> tempVector(temp, temp.size());
				}
				else {
					// Just single value
				}
				break;
			}
			case DatarefType::FLOAT: {
				if (dataref.start_index.has_value()) {
				}
				else {
				}
				break;
			}
			case DatarefType::DOUBLE: {
				break;
			}
			case DatarefType::STRING: {
				break;
			}
			default:
				break;
			}


		}

	}
}

void SubscribeTopic::shutdown()
{
	empty_list();
	//publisher_.reset();
}
