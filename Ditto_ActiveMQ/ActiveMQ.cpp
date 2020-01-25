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
	catch (cms::CMSException & e) {
		//e.printStackTrace();
		XPLMDebugString(e.what());
	}
}

Consumer::Consumer(const std::string& brokerURI, const std::string& destURI, Concurrency::ITarget<std::vector<unsigned char>>& target) :
	connectionFactory(nullptr),
	connection(nullptr),
	session(nullptr),
	destination(nullptr),
	consumer(nullptr),
	brokerURI(brokerURI),
	destURI(destURI),
	_target(target)
{
}

Consumer::~Consumer()
{
	cleanup();
}

void Consumer::cleanup() {
	destination.reset();
	consumer.reset();
	session->close();
	connection->close();
	session.reset();
	connection.reset();
}

void Consumer::onException(const cms::CMSException& ex) {
	XPLMDebugString("CMS Exception occurred. Shutting down client.\n");
	//exit(1);
}

void Consumer::onMessage(const cms::Message* message)
{
	try
	{
		const auto msg = dynamic_cast<const cms::BytesMessage*>(message);
		const auto msg_size = msg->getBodyLength();
		std::unique_ptr<unsigned char> data(msg->getBodyBytes());
		std::vector<unsigned char> buf(data.get(), data.get() + msg_size);

		// Send the message to the buffer
		Concurrency::asend(_target, buf);
	}
	catch (cms::CMSException & e) {
		//e.printStackTrace();
		XPLMDebugString(e.what());
	}
}

void Consumer::run()
{
	try {
		connectionFactory.reset(cms::ConnectionFactory::createCMSConnectionFactory(brokerURI));
		connection.reset(connectionFactory->createConnection());
		connection->start();
		connection->setExceptionListener(this);
		session.reset(connection->createSession(cms::Session::AUTO_ACKNOWLEDGE));
		destination.reset(session->createTopic(destURI));
		consumer.reset(session->createConsumer(destination.get()));
		consumer->setMessageListener(this);
	}
	catch (cms::CMSException & e) {
		//e.printStackTrace();
		XPLMDebugString(e.what());
	}
}