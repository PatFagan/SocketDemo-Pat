
#include "RoboCatPCH.h"

#include <thread>
#include <iostream>
#include <string>
#include <sstream>

#if _WIN32

int DoTCPServer();
int DoTCPClient();


int main(int argc, const char** argv)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
#else
const char** __argv;
int __argc;
int main(int argc, const char** argv)
{
	__argc = argc;
	__argv = argv;
#endif

	// start of class code 1/27 - sockets
	SocketUtil::StaticInit();

	char serverChar;
	std::cout << "Enter 's' for server or 'c' for client: ";
	std::cout.flush();
	std::cin >> serverChar;
	bool isServer = serverChar == 's';

	int result = 0;
	if (isServer)
	{
		result = DoTCPServer();
	}
	else
	{
		result = DoTCPClient();
	}


	


	SocketUtil::CleanUp();

	/*
	OutputWindow win;

	std::thread t([&win]()
				  {
					  int msgNo = 1;
					  while (true)
					  {
						  std::this_thread::sleep_for(std::chrono::milliseconds(250));
						  std::string msgIn("~~~auto message~~~");
						  std::stringstream ss(msgIn);
						  ss << msgNo;
						  win.Write(ss.str());
						  msgNo++;
					  }
				  });

	while (true)
	{
		std::string input;
		std::getline(std::cin, input);
		win.WriteFromStdin(input);
	}

	SocketUtil::CleanUp();
	*/

	return 0;
}

int DoTCPClient()
{
	TCPSocketPtr clientSocket = SocketUtil::CreateTCPSocket(SocketAddressFamily::INET);
	if (clientSocket == nullptr) return 1;

	clientSocket->SetNonBlockingMode(true);

	SocketAddressPtr clientAddress = SocketAddressFactory::CreateIPv4FromString("127.0.0.1::8081");
	if (clientAddress == nullptr) return 1;
	int result = 0;
	result = clientSocket->Bind(*clientAddress);
	if (result != 0) return 1;

	std::cout << "Bound client socket\n";

	SocketAddressPtr serverAddress = SocketAddressFactory::CreateIPv4FromString("127.0.0.1:8080");
	if (serverAddress == nullptr) return 1;

	clientSocket->SetNonBlockingMode(false);
	result = clientSocket->Connect(*serverAddress);
	if (result != 0) return 1;
	clientSocket->SetNonBlockingMode(true);

	clientSocket->Connect(*serverAddress);
	if (result != 0) 
	{
		if (result != -WSAEWOULDBLOCK)
		{
			SocketUtil::ReportError("Client connect");
			return 1;
		}

		result = clientSocket->Connect(*serverAddress);
	}

	std::string message("Hello server!");
	clientSocket->Send(message.c_str(), message.length());

	return 0;
}

int DoTCPServer()
{
	// create the socket on our end to hear data
	TCPSocketPtr listenSocket = SocketUtil::CreateTCPSocket(SocketAddressFamily::INET);
	if (listenSocket == nullptr) return 1; // check if socket is null before continuing

	listenSocket->SetNonBlockingMode(true); // turn blocking mode off

	std::cout << "Created listen socket\n";

	SocketAddressPtr address = SocketAddressFactory::CreateIPv4FromString("127.0.0.1:8080"); // this ip address always maps to local host (me)
	if (address == nullptr) return 1; // check if null

	int result = listenSocket->Bind(*address);
	if (result != 0) return 1; // check if null

	std::cout << "Bound socket\n";

	// if you don't have your socket listen, when other clients try to connect w you, they'll get an error that the host refused connection
	listenSocket->Listen();
	if (listenSocket->Listen() != 0) return 1; // check if null

	std::cout << "Listening on socket\n";

	SocketAddress incomingAddress;
	int error = 0;
	// this function accepts other clients w ip addresses that match the parameter placed here
	TCPSocketPtr connectionSocket = listenSocket->Accept(incomingAddress, error);
	while (connectionSocket != nullptr)
	{
		if (error != WSAEWOULDBLOCK)
		{
			SocketUtil::ReportError("Accepting connection");
			return 1;
		}
		else
		{
			//std::cout << "Failed to accept because socket would block\n";
			connectionSocket = listenSocket->Accept(incomingAddress, error);
		}
	}

	std::cout << "Accepted connection from " << incomingAddress.ToString() << std::endl;

	connectionSocket->SetNonBlockingMode(true); // set blocking mode false on connection socket

	// note: threads run code concurrently (in two diff parts of the pc at once)

	const size_t BUFFSIZE = 4096;
	char buffer[BUFFSIZE];

	// alternative: listenSocket->Receive();
	// this is the socket we made when connecting, it's a diff socket from listenSocket
	// void* is a ptr to any type of ptr, and array name is a ptr to its first index
	int32_t len = connectionSocket->Receive(buffer, BUFFSIZE);
	if (len < 0)
	{
		if (len != -WSAEWOULDBLOCK)
		{
			SocketUtil::ReportError("Receiving data");
			return 1;
		}

		len = connectionSocket->Receive(buffer, BUFFSIZE);
	}

	// cout knows when to stop reading a string bc they all automatically have a null terminator appended at the end
	// so we convert it to a string
	std::string message(buffer, len);
	std::cout << message << std::endl;
}