#include "Server.hpp"

volatile sig_atomic_t g_signalStatus = 1;

void _signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		g_signalStatus = 0;
		std::cout << "\nSIGINT received. Shutting down server gracefully..." << std::endl;
	}
}

int main(int argc, char **argv)
{
	signal(SIGINT, _signalHandler);
	if (argc != 3)
	{
		std::cerr << "Usage: " << argv[0] << " <port> <password>\n" << std::endl;
		return 1;
	}

	try {
		Server server(argv[1], argv[2]);
		server.run();
	} catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}