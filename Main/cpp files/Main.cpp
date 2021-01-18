#include "server_client.h"
#define SERVER "server"
#define CLIENT "client"

int main()
{
    Helper h;
    h.startWinSock();
    
    std::string command;
    getline(cin, command); 
    

    if (command == SERVER) {
        Server server;
        server.run();
    }
    else if (command == CLIENT) {
        Client client;
        client.run();

    }

    std::cout << "Program finished" << std::endl;
    std::cin.get();
    return 0;
}