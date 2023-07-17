#include "../lib/socket.hpp"
#include "../lib/utils.hpp"
#include "../lib/readline.hpp"
#include <thread>
#include <mutex>

mutex mtx;
int serverFD;

void listenFromServer(){
    string message;
    Socket s = Socket(PORT);
    while (true){
        mtx.lock();
        try{
            message = Socket::receive(serverFD);
        } catch(const runtime_error& e){
            mtx.unlock();
            break;
        }

        if(message == "sendIP"){
            message = s.getAddress();
            Socket::send(serverFD, message, 0);
        }else{
            cout << message;
        }
        mtx.unlock();
    }
    return;
}

void sendToServer(){
    string message;

    while (true){
            message = read_line_from_file(stdin);
            Socket::send(serverFD, message, 0);


            if (isCommand(message)) {
                message = Socket::receive(serverFD);
                cout << message << endl;
                if (message == "bye"){
                    return;
                }
            }
        }
}

void Connect()
{
    Socket::send(serverFD, "", 0);
    string message = Socket::receive(serverFD);
    cout << message << endl;
}

int main(){
    signal(SIGINT, sigIntHandler);

    // --- Connecting client to server ---
    cout << "Start chatting with /connect" << endl;
    string message = read_line_from_file(stdin);

    if (!(message.compare("/connect\n"))){
        Socket s = Socket(PORT);
        s.connect();
        serverFD = s.getfileDescriptor();

        // --- Running client ---

        Connect();

        thread t1(listenFromServer);
        thread t2(sendToServer);

        t1.join();
        t2.join();
        return 0;
    }else {
        exit(1);
    }
}

