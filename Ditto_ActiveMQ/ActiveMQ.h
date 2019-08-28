#pragma once
#include <decaf/lang/Runnable.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <activemq/library/ActiveMQCPP.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/TextMessage.h>
#include <cms/BytesMessage.h>
#include <cms/MessageListener.h>
#include <cms/MessageProducer.h>
#include <cms/MessageConsumer.h>
#include <cms/Topic.h>
#include "fmt/format.h"

class Producer : public decaf::lang::Runnable
{
private:
	std::shared_ptr<cms::Connection> connection;
	std::shared_ptr<cms::Session> session;
	std::shared_ptr<cms::Topic> destination;
	std::shared_ptr<cms::MessageProducer> producer;

	std::string brokerURI;
	std::string destURI;

	Producer(const Producer&);
	Producer& operator= (const Producer&);

public:

	Producer(const std::string& brokerURI, const std::string& destURI);
	void close();
	void send_message(std::string& input);
	void send_message(uint8_t* pointer, size_t size);
	void run();
};