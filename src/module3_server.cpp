#include "../lib/utils.hpp"
#include "../lib/readline.hpp"
#include <vector>

#define MAX_CLIENTS 10
#define MAX_CHANNELS 4
#define MAX_NUMBER_CLIENTS 40

vector<int> clients(MAX_NUMBER_CLIENTS);  //  list of all clients on server
vector<int> client_channels(MAX_NUMBER_CLIENTS);  //  channel each client is in
vector<string> client_names(MAX_NUMBER_CLIENTS);  //  list of names of all clients on server
vector<string> channels(MAX_CHANNELS);  // list of channel names
vector<int> occupied_channels(MAX_CHANNELS);    //  0 if not occupied, 1 if available, 2 if full
int number_of_occupied_channels;
vector<int> admin(MAX_CHANNELS);    //  list channel admins (-1 if channel is empty, otherwise client index on clients)
vector<vector<int>> channel_clients(MAX_CHANNELS, vector<int>(MAX_CLIENTS));    //  List of clients by index in each channel (channel first index)
vector<vector<int>> client_muted(MAX_NUMBER_CLIENTS, vector<int>(MAX_CHANNELS));  //  checks if a client is muted or not in a channel
vector<vector<int>> client_kicked(MAX_NUMBER_CLIENTS, vector<int>(MAX_CHANNELS));  //  checks if a client is kicked or not in a channel




void closeConnection(int fd){
    Socket::send(fd, "bye", 0);
    close(fd);
}

void shuts_down_client(int client_index){
    closeConnection(clients[client_index]);
    if(client_channels[client_index] != -1){
        int holder = client_channels[client_index];
        client_channels[client_index] = -1; 

        for(int c = 0; c < MAX_CLIENTS; c++){
            if(channel_clients[holder][c] == client_index){
                channel_clients[holder][c] = -1;
                break;
            }
        }
    }

    for(int c=0; c < MAX_CHANNELS; c++){
        client_muted[client_index][c] = 0;
        client_kicked[client_index][c] = 0;
    }

    clients[client_index] = 0;
    client_channels[client_index] = -1;
    client_names[client_index] = "";

    for(int c; c < MAX_CHANNELS; c++){
        client_muted[client_index][c] = 0;
        client_kicked[client_index][c] = 0;
    }

    return;
}

void shuts_down_channel(int channel_index){
    int err;
    for(int j = 0; j < MAX_CLIENTS; j++){
        err = Socket::send(clients[channel_clients[j][channel_index]], "Channel shutting down.", 0);
        if (err == -1){
            shuts_down_client(clients[j]);
        }
    }
    for(int c=0; c < MAX_NUMBER_CLIENTS; c++){
        if(client_channels[c] == channel_index){
            client_channels[c] = -1;
        }
        client_muted[c][channel_index] = 0;
        client_kicked[c][channel_index] = 0;
    }

    for(int c=0; c < MAX_CLIENTS; c++){
        channel_clients[channel_index][c] = 0;
    }

    admin[channel_index] = -1;
    channels[channel_index] = "";
    occupied_channels[channel_index] = 0;
    number_of_occupied_channels--;
}

string random_nickname(){

    vector<string> consonants = {"b", "c",  "d", "f", "g", "h","j","k","l","m","n","p","q","r","s","t","v","w","x","z"};
    vector<string> extras = {"a","e","o","i","u","y","","r","","l","s","","n","","x","z","","h",""};

    string nickname = "";

    nickname += to_string(rand()%100) + "_";

    for(int ij=0; ij<5; ij++){
        nickname+= consonants[rand()%20];
        nickname += extras[rand()%19];
        nickname += extras[rand()%6];   //  vowels
        nickname += extras[rand()%19];
    }

    nickname += "_" + to_string(rand()%100);


    return nickname;
}


int main(){
    // --- Starting server ---
    Socket s = Socket(PORT);
    s.bind();
    s.listen(MAX_CONNECTIONS);
    int serverFD = s.getfileDescriptor();

    // --- Running server ---
    fd_set fdset;
    int maxFD, currFD, ready, err;
    string message;

    for(int i=0; i<MAX_NUMBER_CLIENTS;i++){
        clients[i] = 0;
        client_names[i] = "";
        client_channels[i] = -1;
        for(int j=0;j<MAX_CHANNELS;j++){
            client_kicked[i][j]=0;
            client_muted[i][j]=0;
        }
    }

    for(int i=0; i<MAX_CHANNELS;i++){
        occupied_channels[i] = 0;
        channels[i] = "";
        admin[i] = -1;
        for(int j=0;j<MAX_CLIENTS;j++){
            channel_clients[i][j]=-1;
        }
    }

    number_of_occupied_channels = 0;

    while (true)
    {
        FD_ZERO(&fdset);

        FD_SET(serverFD, &fdset);
        maxFD = serverFD;

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            // verifies if there's a valid client in i and stores it in fdset
            currFD = clients[i];
            if (currFD)
                FD_SET(currFD, &fdset);

            // save the max value of fd in fdset to use select()
            if (currFD > maxFD)
                maxFD = currFD;
        }

        // wait indefinitely for activity on any socket (timeout is NULL)
        ready = select(maxFD + 1, &fdset, NULL, NULL, NULL);
        if ((ready < 0) && (errno != EINTR)){
            printf("Failed to retrieve the number of ready descriptors");
        }

        // if the socket of the server is ready, there's a connection attempt
        if (FD_ISSET(serverFD, &fdset))
        {
            currFD = s.accept();

            // add new socket to clients[]
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i] == 0)
                {
                    clients[i] = currFD;
                    client_names[i] = random_nickname();
                    break;
                }
            }
        }

        // observing the clients
        for (int i = 0; i < MAX_CLIENTS; i++){
            currFD = clients[i];
            int client_channel = client_channels[i];

            if (FD_ISSET(currFD, &fdset))
            {
                int pongFlag = 0;
                message = Socket::receive(currFD);

                if(isCommand(message)){
                    if(!(message.substr(0,6)).compare("/join ")){
                        string searched_name = message.substr(6);
                        int channel_index = -1;

                        for(int c=0; c < MAX_CHANNELS; c++){    //  Searches for channel
                            if(!searched_name.compare(channels[c])){
                                channel_index = c;
                                break;
                            }
                        }

                        if(channel_index == -1 && number_of_occupied_channels < MAX_CHANNELS){
                            number_of_occupied_channels++;
                            int c;
                            for(c=0;c<MAX_CLIENTS;c++){
                                if(channels[c] == ""){
                                    admin[c] = i;
                                    occupied_channels[c] = 1;
                                    channel_clients[c][0] = i;
                                    channels[c] = searched_name;
                                    break;
                                }
                            }
                            client_channels[i] = c;

                            err = Socket::send(currFD, "Channel created successfully.", 0);
                            if (err == -1){
                                shuts_down_client(i);
                                shuts_down_channel(c);
                            }
                            break;
                        } else if (channel_index == -1){
                                err = Socket::send(currFD, "Channel with name doesn't exist and unable to create new channels. Use /list to see available channels.", 0);
                            if (err == -1){
                                shuts_down_client(i);
                            }
                            break;
                        } else if(occupied_channels[channel_index] != 1){
                            err = Socket::send(currFD, "Channel unavailable. Use /list to see available channels.", 0);
                            if (err == -1){
                                shuts_down_client(i);
                            }
                            break;
                        } else{
                            int counter=0;
                            int j=0;

                            for(j=0;j<MAX_CLIENTS;j++){
                                if(channel_clients[client_channel][j] == i){
                                    channel_clients[client_channel][j] = -1;
                                    break;
                                }
                            }

                            occupied_channels[client_channel] = occupied_channels[client_channel] - (occupied_channels[client_channel]%2);
                            
                            client_channels[i] = channel_index;
                            
                            for(j=0;j<MAX_CLIENTS;j++){
                                if(channel_clients[channel_index][j] == -1){
                                    channel_clients[channel_index][j] = i;
                                    break;
                                }
                            }
                            
                            for(int k=j;k<MAX_CLIENTS;k++){
                                if(channel_clients[channel_index][k] == -1){
                                    counter++;
                                }
                            }
                            
                            if(counter==0){
                                occupied_channels[channel_index]=2;
                            }
                            
                            err = Socket::send(currFD, "Entered Channel succefully.", 0);
                            if (err == -1){
                                shuts_down_client(i);
                            }
                            break;
                        }


                        break;
                    } else if(!((message.substr(0,10)).compare("/nickname "))){
                        string new_nickname = message.substr(10);
                        if(new_nickname.length() >51){ //    50 characters + \n
                                err = Socket::send(currFD, "Invalid new nickname. Set a nickname with size 50 or less.", 0);
                            if (err == -1){
                                shuts_down_client(i);
                            }
                            break;
                        } else{
                            client_names[i] = new_nickname;
                            err = Socket::send(currFD, "new nickname accepted.", 0);
                            if (err == -1){
                                shuts_down_client(i);
                            }
                            break;
                        }
                    } else if(!(message).compare("/list\n")){
                        string available_channels = "[";
                        string ch_name_holder; 

                        for(int c=0; c < MAX_CHANNELS; c++){    //  Prints channel names in a single line
                            if(occupied_channels[c] == 1 ){
                                ch_name_holder = channels[c].substr(0,(channels[c].length()-2));    //  removes \n from channel name
                                available_channels += " { " + channels[c] + " } ";
                            }
                        }

                        available_channels += " ]";

                        err = Socket::send(currFD, available_channels, 0);
                        if (err == -1){
                            if(admin[client_channel] == i){
                                shuts_down_channel(client_channel);
                            }
                            shuts_down_client(i);
                        }
                        break;
                    } else if(!message.compare("/quit\n")){
                        if(admin[client_channel] == i){
                            shuts_down_channel(client_channel);
                        }
                        shuts_down_client(i);
                        break;
                    } else if(client_channel == -1){
                        err = Socket::send(currFD, "Invalid command. Please enter a channel with /join CHANNELNAME. See available channels with /list.", 0);
                        if (err == -1){
                            shuts_down_client(i);
                        }
                    }else if(!message.compare("/ping\n")){
                        err = Socket::send(currFD, "pong", 0);
                        if (err == -1){
                            if(admin[client_channel] == i){
                                shuts_down_channel(client_channel);
                            }
                            shuts_down_client(i);
                        }
                        pongFlag++;
                        break;
                    } else if(admin[client_channel] == i){   //  Admin permission check
                        int client_index = -1;
                        
                        if(!(message.substr(0,6)).compare("/kick ")){

                            string searched_name = message.substr(7);
                            for(int c=0; c < MAX_NUMBER_CLIENTS; c++){    //  Searches for client
                                if(!searched_name.compare(client_names[c])){
                                    client_index = c;
                                    break;
                                }
                            }

                            if(client_index == -1){
                                err = Socket::send(currFD, "Client not found.", 0);
                                if (err == -1){
                                    shuts_down_client(i);
                                    shuts_down_channel(client_channel);
                                }
                            } else if(admin[client_channel] == client_index){
                                err = Socket::send(currFD, "Admin can't ban itself.", 0);
                                if (err == -1){
                                    shuts_down_client(i);
                                    shuts_down_channel(client_channel);
                                }
                            }else if(client_channels[client_index] != client_channel){
                                client_kicked[i][client_channel] = 1;
                                err = Socket::send(currFD, "Client banned from channel.", 0);
                                if (err == -1){
                                    shuts_down_client(i);
                                    shuts_down_channel(client_channel);
                                }
                            } else{
                                client_channels[client_index] = -1;
                                client_kicked[i][client_channel] = 1;
                                occupied_channels[client_index] = occupied_channels[client_index] - (occupied_channels[client_index]%2);
                                err = Socket::send(currFD, "Client banned from channel.", 0);
                                if (err == -1){
                                    shuts_down_client(i);
                                    shuts_down_channel(client_channel);
                                }
                            }

                            break;
                        } else if(!(message.substr(0,6)).compare("/mute ")){
                            string searched_name = message.substr(7);
                            int client_index=-1;
                            for(int c=0; c < MAX_NUMBER_CLIENTS; c++){    //  Searches for client
                                if(!searched_name.compare(client_names[c])){
                                    client_index = c;
                                    break;
                                }
                            }

                            if(client_index == -1){
                                err = Socket::send(currFD, "Client not found.", 0);
                                if (err == -1){
                                    shuts_down_client(i);
                                    shuts_down_channel(client_channel);
                                }
                            } else if(admin[client_channel] == client_index){
                                err = Socket::send(currFD, "Admin can't mute itself.", 0);
                                if (err == -1){
                                    shuts_down_client(i);
                                    shuts_down_channel(client_channel);
                                }
                            } else {
                                client_muted[i][client_channel] = 1;
                                err = Socket::send(currFD, "Client muted in channel.", 0);
                                if (err == -1){
                                    shuts_down_client(i);
                                    shuts_down_channel(client_channel);
                                }
                            }
                            break;
                        } else if(!(message.substr(0,8)).compare("/unmute ")){
                            string searched_name = message.substr(9);
                            int client_index;
                            for(int c=0; c < MAX_NUMBER_CLIENTS; c++){    //  Searches for client
                                if(!searched_name.compare(client_names[c])){
                                    client_index = c;
                                    break;
                                }
                            }

                            if(client_index == -1){
                                err = Socket::send(currFD, "Client not found.", 0);
                                if (err == -1){
                                    shuts_down_client(i);
                                    shuts_down_channel(client_channel);
                                }
                            } else if(admin[client_channel] == client_index){
                                err = Socket::send(currFD, "Admin can't be muted.", 0);
                                if (err == -1){
                                    shuts_down_client(i);
                                    shuts_down_channel(client_channel);
                                }
                            } else {
                                client_muted[i][client_channel] = 0;
                                err = Socket::send(currFD, "Client unmuted in channel.", 0);
                                if (err == -1){
                                    shuts_down_client(i);
                                    shuts_down_channel(client_channel);
                                }
                            }
                            break;
                        } else if(!(message.substr(0,6)).compare("/Whois ")){
                            string searched_name = message.substr(8);
                            int client_index;
                            for(int c=0; c < MAX_NUMBER_CLIENTS; c++){    //  Searches for client
                                if(!searched_name.compare(client_names[c])){
                                    client_index = c;
                                    break;
                                }
                            }

                            if(client_index == -1){
                                err = Socket::send(currFD, "Client not found.", 0);
                                if (err == -1){
                                    shuts_down_client(i);
                                    shuts_down_channel(client_channel);
                                }
                            } else {
                                Socket::send(serverFD, "sendIP", 0);
                                message = Socket::receive(currFD);
                                err = Socket::send(currFD, message, 0);
                                if (err == -1){
                                    shuts_down_client(i);
                                    shuts_down_channel(client_channel);
                                }
                            }
                            break;
                        } else {
                            err = Socket::send(currFD, "invalid command.\n", 0);
                            if (err == -1){
                                if(admin[client_channel] == i){
                                    shuts_down_channel(client_channel);
                                }
                                shuts_down_client(i);
                            }
                        }

                        break;
                    } else {
                        err = Socket::send(currFD, "invalid command.\n", 0);
                        if (err == -1){
                            shuts_down_client(i);
                        }
                    }
                } else if (!pongFlag){
                    message.pop_back();
                    string name = client_names[i];
                    string fmtMessage = name.substr(0,(name.length()-1)) + ": " + message + "\n";
                    int client_holder;
                    for(int j = 0; j < MAX_CLIENTS; j++){
                        client_holder = channel_clients[client_channel][j];
                        err = Socket::send(clients[client_holder], fmtMessage, 0);
                        if (err == -1){
                            if(admin[client_channel] == client_holder){
                                shuts_down_channel(client_channel);
                            }
                            shuts_down_client(client_holder);
                        }
                    }
                }
                pongFlag = 0;
            }
        }
    }
    return 0;
}
