#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <filesystem>
#include <fstream>

using namespace std;
namespace fs = std::filesystem;

string current_path = fs::current_path().string(); // Starting directory

// Function to create a socket
int create_socket() {
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0); // Change to SOCK_DGRAM for UDP
    if (server_socket < 0) {
        cerr << "Failed to create Server Socket.." << endl;
        return -1;
    }
    return server_socket;
}

// Function to bind the socket to an IP and port
bool bind_socket(int server_socket) {
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(21);

    if (bind(server_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Socket Binding Failed.." << endl;
        return false;
    }
    cout << "Socket Binding successful..." << endl;
    return true;
}

// Function to display available commands
void instructions(int client_socket, sockaddr_in client_addr) {
    string help_message =
        "Available Commands:\n"
        "1. HELP - Display this help menu.\n"
        "2. MKDIR <dir_name> - Create a new directory.\n"
        "3. LIST - List files and directories in the current directory.\n"
        "4. PATH - Show the current directory path.\n"
        "5. SAVE <file_name> - Save a file (send file content after command).\nFiles will be Saved in the current diectory and file should exist already in the path where client execution is present.\n"
        "6. RETRIEVE <file_name> - Retrieve a file from server.\nA folder named 'Client' is necessary to be created before using the RETRIEVE command.\nNOTE : If a retrieve fails..it stores an error message in the Folder..\n"
        "7. CD <dir_name> - Change to a specified directory.\n"
        "8. QUIT - Disconnect from the server.\n";

    sendto(client_socket, help_message.c_str(), help_message.length(), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
}

// Function to handle client requests
void handle_client(int server_socket, sockaddr_in client_addr) {
    char buffer[1024];
    socklen_t addr_len = sizeof(client_addr);
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recvfrom(server_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (bytes_received <= 0) {
            cerr << "Client disconnected or error occurred." << endl;
            break;
        }

        string command(buffer);
        command.erase(command.find_last_not_of(" \n\r\t") + 1); // Trim whitespace
        cout << "Received command from client." << endl;

        if (command.empty()) {
            sendto(server_socket, "Command cannot be empty.", 24, 0, (struct sockaddr*)&client_addr, addr_len);
            continue;
        }

        if (command == "HELP") {
            instructions(server_socket, client_addr);
        } else if (command.rfind("MKDIR", 0) == 0) {
            string dir_name = command.substr(6);
            if (fs::create_directory(current_path + "/" + dir_name)) {
                sendto(server_socket, "Directory created.", 18, 0, (struct sockaddr*)&client_addr, addr_len);
            } else {
                sendto(server_socket, "Failed to create directory or already exists.", 44, 0, (struct sockaddr*)&client_addr, addr_len);
            }
        } else if (command == "LIST") {
            string contents;
            for (const auto& entry : fs::directory_iterator(current_path)) {
                contents += entry.path().filename().string() + "\n";
            }
            if (contents.empty()) {
                contents = "Directory is empty.";
            }
            sendto(server_socket, contents.c_str(), contents.length(), 0, (struct sockaddr*)&client_addr, addr_len);
        } else if (command == "PATH") {
            sendto(server_socket, current_path.c_str(), current_path.length(), 0, (struct sockaddr*)&client_addr, addr_len);
        } else if (command.rfind("SAVE", 0) == 0) {
            string file_name = command.substr(5);
            memset(buffer, 0, sizeof(buffer));
            int file_bytes = recvfrom(server_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&client_addr, &addr_len);
            if (file_bytes > 0) {
                ofstream outfile(current_path + "/" + file_name, ios::binary);
                outfile.write(buffer, file_bytes);
                outfile.close();
                cout << "File saved at: " << current_path + "/" + file_name << endl;
                sendto(server_socket, "File saved.", 12, 0, (struct sockaddr*)&client_addr, addr_len);
            } else {
                sendto(server_socket, "No file data received.", 22, 0, (struct sockaddr*)&client_addr, addr_len);
            }
        } else if (command.rfind("RETRIEVE", 0) == 0) {
            string file_name = command.substr(9);
            ifstream infile(current_path + "/" + file_name, ios::binary);
            if (!infile) {
                string error_message = "File not found: " + file_name;
                cout << error_message << endl;
                ofstream error_file("Client/error_" + file_name + ".txt");
                error_file << error_message;
                error_file.close();
                sendto(server_socket, "File not found.", 15, 0, (struct sockaddr*)&client_addr, addr_len);
            } else {
                string file_contents((istreambuf_iterator<char>(infile)), (istreambuf_iterator<char>()));
                infile.close();
                sendto(server_socket, file_contents.c_str(), file_contents.length(), 0, (struct sockaddr*)&client_addr, addr_len);
                cout << "File retrieved: " << current_path + "/" + file_name << endl;
            }
        } else if (command.rfind("CD", 0) == 0) {
            string dir_name = command.substr(3);
            fs::path new_path = fs::path(current_path) / dir_name;
            if (fs::exists(new_path) && fs::is_directory(new_path)) {
                current_path = new_path.string();
                sendto(server_socket, "Changed directory.", 17, 0, (struct sockaddr*)&client_addr, addr_len);
            } else {
                sendto(server_socket, "Directory not found.", 20, 0, (struct sockaddr*)&client_addr, addr_len);
            }
        } else if (command == "QUIT") {
            sendto(server_socket, "Disconnecting.", 14, 0, (struct sockaddr*)&client_addr, addr_len);
            break;
        } else {
            sendto(server_socket, "Unknown command.", 16, 0, (struct sockaddr*)&client_addr, addr_len);
        }
    }
}

// Function to set up the server
void setup_server() {
    int server_socket = create_socket();
    if (!bind_socket(server_socket)) return;
    cout << "Waiting for client connections..." << endl;
    sockaddr_in client_addr;
    while (true) {
        handle_client(server_socket, client_addr);
    }
}

// Main function to run the server
int main() {
    setup_server();
    return 0;
}
