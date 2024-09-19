#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>

using namespace std;

// Function to create a socket
int create_socket() {
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0); // Change to SOCK_DGRAM for UDP
    if (client_socket < 0) {
        cerr << "Failed to create Client Socket.." << endl;
        return -1;
    }
    return client_socket;
}

// Function to connect to the server (setup for UDP)
bool connect_to_server(int client_socket, const string& server_ip, int server_port) {
    return true; // No actual connection for UDP
}

// Function to handle sending commands to the server
void send_commands(int client_socket, sockaddr_in server_addr) {
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
            sendto(client_socket, command.c_str(), command.length(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
            cout << "Disconnecting from server." << endl;
            break;
        }

        if (command.rfind("SAVE", 0) == 0) {
            string file_name = command.substr(5);
            ifstream infile(file_name, ios::binary);
            if (infile) {
                sendto(client_socket, command.c_str(), command.length(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                string file_contents((istreambuf_iterator<char>(infile)), (istreambuf_iterator<char>()));
                sendto(client_socket, file_contents.c_str(), file_contents.length(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                cout << "File saved at server: " << file_name << endl;
            } else {
                cout << "File not found!" << endl;
            }
            continue;
        } else if (command.rfind("RETRIEVE", 0) == 0) {
            sendto(client_socket, command.c_str(), command.length(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            socklen_t addr_len = sizeof(server_addr);
            int bytes_received = recvfrom(client_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&server_addr, &addr_len);
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

        sendto(client_socket, command.c_str(), command.length(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        socklen_t addr_len = sizeof(server_addr);
        int bytes_received = recvfrom(client_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&server_addr, &addr_len);
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
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    send_commands(client_socket, server_addr);

    close(client_socket);
    return 0;
}
