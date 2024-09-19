#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>

using namespace std;

// Function to create a socket
int create_socket() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        cerr << "Failed to create Client Socket.." << endl;
        return -1;
    }
    return client_socket;
}

// Function to connect to the server
bool connect_to_server(int client_socket, const string& server_ip, int server_port) {
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Connection to server failed.." << endl;
        return false;
    }
    cout << "Connected to server at " << server_ip << ":" << server_port << endl;
    return true;
}

// Function to handle sending commands to the server
void send_commands(int client_socket) {
    string command;
    cout << "---------------- Maximum File transfer size 1KB ---------------------" << endl;
    cout << "Type HELP to display commands available ..." << endl;

    while (true) {
        cout << "> ";
        getline(cin, command);
        command.erase(command.find_last_not_of(" \n\r\t") + 1); // Trim whitespace

        if (command.empty()) {
            cout << "Command cannot be empty." << endl;
            continue;
        }

        if (command == "QUIT") {
            send(client_socket, command.c_str(), command.length(), 0);
            cout << "Disconnecting from server." << endl;
            break;
        }

        if (command.rfind("SAVE", 0) == 0) {
            string file_name = command.substr(5);
            ifstream infile(file_name, ios::binary);
            if (infile) {
                send(client_socket, command.c_str(), command.length(), 0);
                string file_contents((istreambuf_iterator<char>(infile)), (istreambuf_iterator<char>()));
                send(client_socket, file_contents.c_str(), file_contents.length(), 0);
                cout << "File saved at server: " << file_name << endl;
            } else {
                cout << "File not found!" << endl;
            }
            continue;
        } else if (command.rfind("RETRIEVE", 0) == 0) {
            send(client_socket, command.c_str(), command.length(), 0);
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0'; // Null-terminate the received data
                if (strncmp(buffer, "File not found.", 15) == 0) {
                    cout << "File not found on server. Saving error message." << endl;
                    ofstream error_file("Client/error_" + command.substr(9) + ".txt");
                    error_file << buffer;
                    error_file.close();
                } else {
                    ofstream outfile("Client/" + command.substr(9), ios::binary);
                    outfile.write(buffer, bytes_received);
                    outfile.close();
                    cout << "File retrieved and saved at: Client/" + command.substr(9) << endl;
                }
            } else {
                cout << "No response from server." << endl;
            }
            continue;
        }

        send(client_socket, command.c_str(), command.length(), 0);

        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; // Null-terminate the received data
            cout << "Server response:\n" << buffer << endl;
        } else {
            cout << "No response from server." << endl;
        }
    }
}

// Main function to run the client
int main() {
    string server_ip = "127.0.0.1";
    int server_port = 21;

    int client_socket = create_socket();
    if (!connect_to_server(client_socket, server_ip, server_port)) return 1;

    send_commands(client_socket);

    close(client_socket);
    return 0;
}
