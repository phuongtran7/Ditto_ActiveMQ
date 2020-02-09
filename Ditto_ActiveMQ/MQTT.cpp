#include "MQTT.h"

MQTT_Publisher::MQTT_Publisher(const std::string& address, const std::string& topic, int qos) :
	address_(address),
	topic_(topic),
	qos_(qos),
	client_(address_, ""), // Force random clientID
	conn_options_{}
{
	conn_options_.set_clean_session(true);
	client_.set_callback(*this);
	client_.connect(conn_options_)->wait();
}

MQTT_Publisher::~MQTT_Publisher()
{
	client_.disconnect()->wait();
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
