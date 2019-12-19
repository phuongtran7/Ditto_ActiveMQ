#include "ActiveMQ.h"
#include "XPLMUtilities.h"

Producer::Producer(const std::string& brokerURI, const std::string& destURI) :
	connectionFactory(nullptr),
	connection(nullptr),
	session(nullptr),
	destination(nullptr),
	producer(nullptr),
	brokerURI(brokerURI),
	destURI(destURI)
{
}

void Producer::cleanup() {
	destination.reset();
	producer.reset();
	session->close();
	connection->close();
	session.reset();
	connection.reset();
	connectionFactory.reset();
}

void Producer::send_message(std::string& input) {
	auto msg = std::unique_ptr<cms::TextMessage>(session->createTextMessage(input));
	producer->send(msg.get());
}

void Producer::send_message(uint8_t* pointer, size_t size) {
	auto msg = std::unique_ptr<cms::BytesMessage>(session->createBytesMessage(pointer, size));
	producer->send(msg.get());
}

void Producer::send_message(const std::vector<uint8_t>& pointer, size_t size)
{
	auto msg = std::unique_ptr<cms::BytesMessage>(session->createBytesMessage(pointer.data(), size));
	producer->send(msg.get());
}

void Producer::run() {
	try {
		connectionFactory.reset(cms::ConnectionFactory::createCMSConnectionFactory(brokerURI));
		connection.reset(connectionFactory->createConnection());
		session.reset(connection->createSession(cms::Session::AUTO_ACKNOWLEDGE));
		destination.reset(session->createTopic(destURI));
		producer.reset(session->createProducer(destination.get()));
		producer->setDeliveryMode(cms::DeliveryMode::NON_PERSISTENT);
		connection->start();
	}
	catch (cms::CMSException& e) {
		//e.printStackTrace();
		XPLMDebugString(e.what());
	}
}