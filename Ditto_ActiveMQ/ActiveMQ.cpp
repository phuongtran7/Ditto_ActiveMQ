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

void Producer::send_message(const std::string& input) {
	try {
		auto msg = std::unique_ptr<cms::TextMessage>(session->createTextMessage(input));
		producer->send(msg.get());
	}
	catch (const cms::CMSException & e) {
		XPLMDebugString(e.what());
		XPLMDebugString("\n");
	}
	catch (const cms::MessageFormatException & e) {
		XPLMDebugString(e.what());
		XPLMDebugString("\n");
	}
	catch (const cms::InvalidDestinationException & e) {
		XPLMDebugString(e.what());
		XPLMDebugString("\n");
	}
	catch (const cms::UnsupportedOperationException & e) {
		XPLMDebugString(e.what());
		XPLMDebugString("\n");
	}
}

void Producer::send_message(uint8_t* pointer, size_t size) {
	if (size != 0) {
		try {
			auto msg = std::unique_ptr<cms::BytesMessage>(session->createBytesMessage(pointer, size));
			producer->send(msg.get());
		}
		catch (const cms::CMSException & e) {
			XPLMDebugString(e.what());
			XPLMDebugString("\n");
		}
		catch (const cms::MessageFormatException & e) {
			XPLMDebugString(e.what());
			XPLMDebugString("\n");
		}
		catch (const cms::InvalidDestinationException & e) {
			XPLMDebugString(e.what());
			XPLMDebugString("\n");
		}
		catch (const cms::UnsupportedOperationException & e) {
			XPLMDebugString(e.what());
			XPLMDebugString("\n");
		}
	}
	else {
		XPLMDebugString("Ditto: Message is empty.\n.");
		return;
	}
}

void Producer::send_message(const std::vector<uint8_t>& pointer, size_t size)
{
	if (size != 0) {
		try {
			auto msg = std::unique_ptr<cms::BytesMessage>(session->createBytesMessage(pointer.data(), size));
			producer->send(msg.get());
		}
		catch (const cms::CMSException & e) {
			XPLMDebugString(e.what());
			XPLMDebugString("\n");
		}
		catch (const cms::MessageFormatException & e) {
			XPLMDebugString(e.what());
			XPLMDebugString("\n");
		}
		catch (const cms::InvalidDestinationException & e) {
			XPLMDebugString(e.what());
			XPLMDebugString("\n");
		}
		catch (const cms::UnsupportedOperationException & e) {
			XPLMDebugString(e.what());
			XPLMDebugString("\n");
		}
	}
	else {
		XPLMDebugString("Ditto: Message is empty.\n.");
		return;
	}
}

void Producer::run() {
	try {
		connectionFactory.reset(cms::ConnectionFactory::createCMSConnectionFactory(brokerURI));
		connection.reset(connectionFactory->createConnection());
		connection->start();
		session.reset(connection->createSession(cms::Session::AUTO_ACKNOWLEDGE));
		destination.reset(session->createTopic(destURI));
		producer.reset(session->createProducer(destination.get()));
		producer->setDeliveryMode(cms::DeliveryMode::NON_PERSISTENT);
		producer->setTimeToLive(1800000); // 30 minutes time to live message
	}
	catch (cms::CMSException & e) {
		XPLMDebugString(fmt::format("Ditto: {}", e.what()).c_str());
	}
}