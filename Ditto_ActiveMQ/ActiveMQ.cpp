#include "ActiveMQ.h"
#include "XPLMUtilities.h"

Producer::Producer(const std::string& brokerURI, const std::string& destURI) :
	connection(nullptr),
	session(nullptr),
	destination(nullptr),
	producer(nullptr),
	brokerURI(brokerURI),
	destURI(destURI)
{
}

void Producer::close() {
	try {
		if (connection != NULL) {
			connection->close();
		}
	}
	catch (cms::CMSException& e) {
		//e.printStackTrace();
		XPLMDebugString(e.what());
	}

	destination.reset();
	producer.reset();
	session.reset();
	connection.reset();
}

void Producer::send_message(std::string& input) {
	auto msg = std::unique_ptr<cms::TextMessage>(session->createTextMessage(input));
	producer->send(msg.get());
}

void Producer::send_message(uint8_t* pointer, size_t size) {
	auto msg = std::unique_ptr<cms::BytesMessage>(session->createBytesMessage(pointer, size));
	producer->send(msg.get());
}

void Producer::run() {
	try {
		std::unique_ptr <activemq::core::ActiveMQConnectionFactory> connectionFactory(new activemq::core::ActiveMQConnectionFactory(brokerURI));

		connection.reset(connectionFactory->createConnection());

		// Use reset to obtain the ownership of the new object
		// https://stackoverflow.com/questions/13311580/error-using-stdmake-shared-abstract-class-instantiation
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