#include <iostream>
#include <thread>

#include "Server.h"

int main()
{
	Server server;
	server.Init();

	std::thread runThread(
	// CPP: lambda expression
	// [...] : capture block
	// () : parameter list
	// {} : function body
		[&]()
		{
			server.Run();
		}
	);

	std::cout << "press any key to exit...\n";
	getchar();

	server.Stop();
	runThread.join();

	return 0;
}