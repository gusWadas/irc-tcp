#include "../lib/socket.hpp"
#include "../lib/utils.hpp"
#include "../lib/readline.hpp"
#include <thread>
#include <mutex>

mutex mtx;
int serverFD;

void listenFromServer()
{
    string message;
    while (true)
    {
        mtx.lock();
        try{
            message = Socket::receive(serverFD);
        } catch(const runtime_error& e){
            mtx.unlock();
            break;
        }
        cout << message << endl;
        mtx.unlock();
    }
    return;
}

void Connect()
{
    Socket::send(serverFD, "", 0);
    string message = Socket::receive(serverFD);
    cout << message << endl;
}

int main()
{
    signal(SIGINT, sigIntHandler);

    // --- Connecting client to server ---
    cout << "Start chatting with /connect" << endl;
    string message = read_line_from_file(stdin);

    if (!(message.compare("/connect")))
    {
        Socket s = Socket(PORT);
        s.connect();
        serverFD = s.getfileDescriptor();

        // --- Running client ---

        Connect();

        thread t1(listenFromServer);

        while (true)
        {

            message = read_line_from_file(stdin);
            Socket::send(serverFD, message, 0);


            if (isCommand(message))
            {
                message = Socket::receive(serverFD);
                cout << message << endl;
                if (message == "bye"){
                    t1.detach();
                    return 0;
                }
            }
        }

        t1.detach();
        return 0;
    }
    else
        exit(1);
}
