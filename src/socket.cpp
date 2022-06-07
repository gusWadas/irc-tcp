#include "../lib/socket.hpp"

Socket::Socket(unsigned short port) {

    // Socket creation
    // AF_INET for IPv4; SOCK_STREAM for TCP; 0 for IP
    this->fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

    // Socket error handling
    if (this->fileDescriptor == -1) {
        throw runtime_error("Failed to create TCP socket");
    }

    // Socket address definition
    this->address.sin_family = AF_INET;
    this->address.sin_addr.s_addr = INADDR_ANY;
    this->address.sin_port = htons(port); // avoid endianness problems
}

void Socket::bind()
{
    int reuse = 1;
    if (setsockopt(this->fileDescriptor, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse) == -1)
    {
        throw runtime_error("Failed to define socket options");
    }

    if (::bind(this->fileDescriptor, (struct sockaddr *)&this->address, sizeof(this->address)) == -1)
    {
        throw runtime_error("Failed to bind socket with the given address");
    }
}


Socket::~Socket()
{
    close(this->fileDescriptor);
}

void Socket::listen(int maxConnections)
{
    if (::listen(this->fileDescriptor, maxConnections) == -1)
    {
        throw runtime_error("Failed to listen to socket");
    }

    return;
}

int Socket::accept()
{
    int client;
    struct sockaddr_storage address;
    socklen_t addressSize = sizeof address;

    client = ::accept(this->fileDescriptor, (struct sockaddr *)&address, &addressSize);

    return client;
}

void Socket::connect()
{
    if (::connect(this->fileDescriptor, (struct sockaddr *)&this->address, sizeof(this->address)) == -1)
    {
        throw runtime_error("Failed to connect to socket");
    }

    return;
}

int Socket::send(int fileDescriptor, string msg)
{
    int nMessages = 0, bytes;

    while (msg.size())
    {
        string substr;

        if (msg.size() < MAX_MESSAGE_SIZE)
        {
            substr = msg;
            msg.clear();
        }
        else
        {
            substr = msg.substr(0, MAX_MESSAGE_SIZE);
            msg = msg.substr(MAX_MESSAGE_SIZE);
        }

        bytes = ::send(fileDescriptor, substr.c_str(), substr.size(), 0);
        nMessages++;

        if (bytes == -1)
        {
            throw runtime_error("Failed to send message");
        }
    }
    return nMessages;
}

string Socket::receive(int fileDescriptor)
{
    char buffer[MAX_MESSAGE_SIZE + 1];
    memset(buffer, 0, MAX_MESSAGE_SIZE);

    if (::recv(fileDescriptor, buffer, MAX_MESSAGE_SIZE, 0)<0)
    {
        throw runtime_error("Failed to receive message");
    }

    cout << fileDescriptor << ": " << buffer << endl;
    return string(buffer);
}

int Socket::getfileDescriptor()
{
    return this->fileDescriptor;
}