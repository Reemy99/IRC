#include "server.hpp"

Server::Server() : _buffer("\0")
{
    std::cout << GREEN << "Server Constructor Called" << RESET << std::endl;
}

Server::~Server()
{
    std::cout << RED << "Server Destructor Called" << RESET << std::endl;
}

int Server::createSocket()
{
    //int socket(int domain, int type, int protocol);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        std::cerr << RED "Failed to create socket" << RESET << std::endl;
        exit (EXIT_FAILURE);
    }
    std::cout << GREEN << "Socket Created" << RESET << std::endl;
    return sockfd;
}

void Server::bindSocket(int sockfd)
{
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; //(0.0. 0.0) any address for binding
    serverAddr.sin_port = htons(PORT); // convert to network byte order.

    // int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        std::cerr << RED << "Failed to bind socket." << RESET << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << GREEN << "Bind socket has been successfully bound." << RESET << std::endl;
}

void Server::listenSocket(int sockfd)
{
    if (listen(sockfd , MAX_CLIENTS) == - 1)
    {
        std::cerr << RED << "Failed to listen socket." << RESET << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << GREEN << "Successfully listen " << RESET << std::endl;
}

int Server::acceptConection(int sockfd)
{
    struct sockaddr_in clientAddr; //hold clientAddr information
    socklen_t clientLen = sizeof (clientAddr);
    //int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
    int clientFd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientLen);
    if (clientFd == -1){
        std::cerr << RED << "Failed to accept << " << RESET << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << GREEN << "Successfully accept " << RESET << std::endl;
    return clientFd; //Return the new socket descriptor for communication with the client.
}

void Server::removeUser(std::vector<User> &users, int fd)
{
    std::vector<User>::iterator it = std::find_if(users.begin(), users.end(), FindByFD(fd));
    if (it != users.end()) {
        close(it->_fd);
        users.erase(it);
    }
}

void Server::runServer()
{
    int sockfd = createSocket();
    bindSocket(sockfd);
    listenSocket(sockfd);


    pollfd tmp = {sockfd, POLLIN, 0};
    _fds.push_back(tmp);

    while(true)
    {
        //int poll(representing a FD, number of FD, timeout);
        int numFds = poll(_fds.data(), _fds.size(), -1);
        if (numFds == -1)
        {
            std::cout << RED "failed to poll" << RESET << std::endl;
            exit (EXIT_FAILURE);
        }
        for (int i = 0 ; i < (int)_fds.size(); i++){
            if (_fds[i].revents & POLLIN) { //data that can be read without blocking AND can safely read operation be on it
                if (_fds[i].fd == sockfd) {
                    // New client connection and add it to "users, _fds" vectors
                    int clientFd = acceptConection(sockfd);
                    pollfd tmp2 = {clientFd, POLLIN, 0};
                    _fds.push_back(tmp2);
                    _users.push_back(User(clientFd));
                    std::cout << BLUE << "new client connected FD:" << clientFd << RESET << std::endl;
                }
                else{
                    // Client message received
                    int byteRead = read(_fds[i].fd, _buffer, sizeof(_buffer));
                    _buffer[byteRead - 1] = '\0';
                    std::cout << "---------> " << byteRead << std::endl;
                    if (byteRead <= 0){
                        std::cout << RED << "Client disconnected FD :" << _fds[i].fd << RESET << std::endl;
                        removeUser(_users, _fds[i].fd);
                        _fds.erase(_fds.begin() + i);
                        i--;
                    }
                    else {
                        std::vector<User>::iterator it = std::find_if(_users.begin(), _users.end(), FindByFD(_fds[i].fd));

                        std::string strBuffer(_buffer);
                        std::cout << BLUE << "Received message from client" << _fds[i].fd << ": " << RESET << strBuffer << std::endl;
                        it->parse(strBuffer);
                    }
                }
            }
        }
    }
}
