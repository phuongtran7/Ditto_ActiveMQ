#pragma once
#include <decaf/lang/Runnable.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <activemq/transport/DefaultTransportListener.h>
#include <activemq/library/ActiveMQCPP.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/TextMessage.h>
#include <cms/BytesMessage.h>
#include <cms/MessageProducer.h>
#include <cms/ExceptionListener.h>
#include <cms/MessageListener.h>
#include <cms/Topic.h>
#include "fmt/format.h"
#include <agents.h>

class Producer : public decaf::lang::Runnable
{
private:
	std::unique_ptr<cms::ConnectionFactory> connectionFactory;
	std::unique_ptr<cms::Connection> connection;
	std::unique_ptr<cms::Session> session;
	std::unique_ptr<cms::Topic> destination;
	std::unique_ptr<cms::MessageProducer> producer;
	std::string brokerURI{};
	std::string destURI{};
public:
	explicit Producer(const std::string& brokerURI, const std::string& destURI);
	void cleanup();
	void send_message(std::string& input);
	void send_message(uint8_t* pointer, size_t size);
	void send_message(const std::vector<uint8_t>& pointer, size_t size);
	void run();
};

class Consumer : public cms::ExceptionListener,
	public cms::MessageListener,
	public activemq::transport::DefaultTransportListener
{
private:
	std::unique_ptr<cms::ConnectionFactory> connectionFactory;
	std::unique_ptr<cms::Connection> connection;
	std::unique_ptr<cms::Session> session;
	std::unique_ptr<cms::Topic> destination;
	std::unique_ptr<cms::MessageConsumer> consumer;
	std::string brokerURI{};
	std::string destURI{};
	Concurrency::ITarget<std::vector<unsigned char>>& _target;
public:
	explicit Consumer(const std::string& brokerURI, const std::string& destURI, Concurrency::ITarget<std::vector<unsigned char>>& target);
	~Consumer();
	void cleanup();
	void run();
	void onException(const cms::CMSException& ex) override;
	void onMessage(const cms::Message* message) override;
};