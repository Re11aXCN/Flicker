#include <print>
#include <json/json.h>

#include "FKServer.h"

int main(int argc, char* argv[])
{
	try
	{
		UINT16 port = 8080;
		boost::asio::io_context ioContext;
		boost::asio::signal_set signals(ioContext, SIGINT, SIGTERM);
		signals.async_wait([&ioContext](const boost::system::error_code& error, int signalNumber) {
			 if (error)
			 {
				 std::println("Error: {}", error.message());
				 return;
			 }
			 ioContext.stop();
			});
		std::make_shared<FKServer>(ioContext, port)->start();
		ioContext.run();
	}
	catch (const std::exception& e)
	{
		std::println("EXCEPTION: {}", e.what());
		return EXIT_FAILURE;
	}
}