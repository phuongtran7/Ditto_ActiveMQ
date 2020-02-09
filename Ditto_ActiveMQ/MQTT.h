#pragma once
#include "mqtt/async_client.h"
#include <mqtt/callback.h>
#include "fmt/format.h"

class MQTT_Publisher : public virtual mqtt::callback,
	public virtual mqtt::iaction_listener
{
public:
	MQTT_Publisher(const std::string& address, const std::string& topic, int qos);
	~MQTT_Publisher();

	void send_message(const std::string& message);
	void send_message(const std::vector<uint8_t>& pointer, size_t size);

private:
	std::string address_;
	std::string topic_;
	int qos_;
	mqtt::async_client client_;
	mqtt::connect_options conn_options_;

private:
	void delivery_complete(mqtt::delivery_token_ptr token) override;
	void on_failure(const mqtt::token& tok) override;
	void on_success(const mqtt::token& tok) override;
	void connected(const std::string& cause) override;
	void connection_lost(const std::string& cause) override;
};
