#include "MQTT.h"

MQTT_Publisher::MQTT_Publisher(const std::string& address, const std::string& topic, int qos) :
	address_(address),
	topic_(topic),
	qos_(qos),
	client_(address_, ""), // Force random clientID
	conn_options_{}
{
	try {
		conn_options_.set_clean_session(true);
		client_.set_callback(*this);
		client_.connect(conn_options_)->wait();
	}
	catch (const mqtt::exception & exc) {
		fmt::print(exc.what());
	}
}

MQTT_Publisher::~MQTT_Publisher()
{
	try {
		client_.disconnect()->wait();
	}
	catch (const mqtt::exception & exc) {
		fmt::print(exc.what());
	}
}

void MQTT_Publisher::send_message(const std::string& message)
{
	mqtt::message_ptr pubmsg = mqtt::make_message(topic_, message.c_str(), message.size(), 0, false);
	client_.publish(pubmsg, nullptr, *this);
}

void MQTT_Publisher::send_message(const std::vector<uint8_t>& pointer, size_t size)
{
	mqtt::message_ptr pubmsg = mqtt::make_message(topic_, pointer.data(), size, 0, false);
	client_.publish(pubmsg, nullptr, *this);
}

void MQTT_Publisher::delivery_complete(mqtt::delivery_token_ptr token)
{
	fmt::print("Delivered message.\n");
}

void MQTT_Publisher::on_failure(const mqtt::token& tok)
{
	fmt::print("failed.\n");
}

void MQTT_Publisher::on_success(const mqtt::token& tok)
{
	fmt::print("Succeed.\n");
}

void MQTT_Publisher::connected(const std::string& cause)
{
	fmt::print("Connection success. Will publish to {}.\n", topic_);
}

void MQTT_Publisher::connection_lost(const std::string& cause)
{
	fmt::print("Connection lost: {}.\nReconnecting...\n", cause);
	client_.reconnect()->wait();
}

MQTT_Subscriber::MQTT_Subscriber(const std::string& address, const std::string& topic, int qos) :
	address_(address),
	topic_(topic),
	qos_(qos),
	client_(address_, ""), // Force random clientID
	conn_options_{}
{
	try {
		conn_options_.set_keep_alive_interval(20);
		conn_options_.set_clean_session(true);
		client_.set_callback(*this);
		client_.connect(conn_options_)->wait();
	}
	catch (const mqtt::exception & exc) {
		fmt::print(exc.what());
	}
}

MQTT_Subscriber::~MQTT_Subscriber()
{
	try {
		client_.unsubscribe(topic_)->wait();
		client_.stop_consuming();
		client_.disconnect()->wait();
	}
	catch (const mqtt::exception & exc) {
		fmt::print(exc.what());
	}
}

void MQTT_Subscriber::message_arrived(mqtt::const_message_ptr msg)
{
	//fmt::print("Topic: {} - Payload: {}\n", msg->get_topic(), msg->to_string());
	auto payload = msg->get_payload();
	auto recieved = flexbuffers::GetRoot(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()).AsFloat();
	fmt::print("Got: {}\n", recieved);
}

void MQTT_Subscriber::on_failure(const mqtt::token& tok)
{
	fmt::print("Connection attempt failed. Will try to reconnect.\n");
	client_.reconnect()->wait();
}

void MQTT_Subscriber::on_success(const mqtt::token& tok)
{
	fmt::print("Succeed.\n");
}

void MQTT_Subscriber::connected(const std::string& cause)
{
	fmt::print("Connection success. Will subscribte to {}.\n", topic_);
	client_.subscribe(topic_, qos_, nullptr, *this);
}

void MQTT_Subscriber::connection_lost(const std::string& cause)
{
	fmt::print("Connection lost: {}.\nReconnecting...\n", cause);
	client_.reconnect()->wait();
}
