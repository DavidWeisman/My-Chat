#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <bitset>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include "MyChat.h"
#include <thread>
#include <mutex>
#include <conio.h> 
#include<fstream>
#pragma comment(lib, "Ws2_32.lib")

#define MAX_BUF_SIZE 512

std::mutex consoleMutex;
using namespace std;

//-------------------------------Serializer Class-----------------------

std::vector<char> Serializer::serialize(Message msg) {
    std::vector<char> vec;

    std::string sender = msg.getSender();
    std::string content = msg.getContent();
    std::chrono::system_clock::time_point timestamp = msg.getTimeStamp();
    std::time_t time_t_timestamp = std::chrono::system_clock::to_time_t(timestamp);
    MessageType type = msg.getType();
    MessageColor color = msg.getColor();

    int senderLen = sender.length();
    int contentLen = content.length();

    Serializer::intToBinary(senderLen, vec, 31);
    Serializer::stringToBinary(sender, vec);
    Serializer::intToBinary(contentLen, vec, 31);
    Serializer::stringToBinary(content, vec);
    Serializer::intToBinary(time_t_timestamp, vec, 63);


    switch (type)
    {
    case TEXT:
        Serializer::intToBinary(1, vec, 31);
        break;
    case COMMAND:
        Serializer::intToBinary(2, vec, 31);
        break;
    case FILEE:
        Serializer::intToBinary(3, vec, 31);
        break;
    default:
        Serializer::intToBinary(0, vec, 31);
        break;
    }

    switch (color)
    {
    case BLUE:
        Serializer::intToBinary(9, vec, 31);
        break;
    case GREEN:
        Serializer::intToBinary(10, vec, 31);
        break;
    case LIGHT_BLUE:
        Serializer::intToBinary(11, vec, 31);
        break;
    case RED:
        Serializer::intToBinary(12, vec, 31);
        break;
    case PINCK:
        Serializer::intToBinary(13, vec, 31);
        break;
    case YELLOW:
        Serializer::intToBinary(14, vec, 31);
        break;
    default:
        Serializer::intToBinary(15, vec, 31);
        break;
    }

    return vec;
}


Message Serializer::deserialize(std::vector<char> vec) {

    int senderLen = binaryToInt(vec, 0, 31);
    std::string sender = binaryTostring(vec, senderLen, 32);

    int contentLen = binaryToInt(vec, 32 + senderLen * 8, 31);
    std::string content = binaryTostring(vec, contentLen, 64 + senderLen * 8);

    std::time_t time_t_timestamp = binaryToInt(vec, 64 + (senderLen + contentLen) * 8, 63);
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::from_time_t(time_t_timestamp);

    int intType = binaryToInt(vec, 128 + (senderLen + contentLen) * 8, 31);
    MessageType type;

    switch (intType)
    {
    case 1:
        type = TEXT;
        break;
    case 2:
        type = COMMAND;
        break;
    case 3:
        type = FILEE;
        break;
    default:
        type = TEXT;
        break;
    }

    int intColor = binaryToInt(vec, 160 + (senderLen + contentLen) * 8, 31);
    MessageColor color;

    switch (intColor)
    {
    case 9:
        color = BLUE;
        break;
    case 10:
        color = GREEN;
        break;
    case 11:
        color = LIGHT_BLUE;
        break;
    case 12:
        color = RED;
        break;
    case 13:
        color = PINCK;
        break;
    case 14:
        color = YELLOW;
        break;
    default:
        color = WHITE;
        break;
    }

    Message msg(sender, content, timestamp, type, color);
    return msg;
}

void Serializer::intToBinary(int inputNumber, std::vector<char>& vec, int numberSize) {
    std::string binaryStr;

    for (int i = numberSize; i >= 0; --i) {
        if (inputNumber & (1 << i)) {
            binaryStr += '1';
        }
        else {
            binaryStr += '0';
        }
    }

    for (int i = 0; i < binaryStr.size(); i++) {
        vec.push_back(binaryStr[i]);
    }
}

void Serializer::stringToBinary(const std::string& inputString, std::vector<char>& vec) {
    std::string binaryStr;

    for (char ch : inputString) {
        binaryStr += std::bitset<8>(ch).to_string();
    }

    for (int i = 0; i < binaryStr.size(); i++) {
        vec.push_back(binaryStr[i]);
    }
}

std::string Serializer::binaryTostring(std::vector<char>& vec, int lenOfString, int index) {
    std::string str;
    for (int j = 0; j < lenOfString; j++) {
        char parsed = 0;
        for (int i = 0; i < 8; i++) {
            if (vec[index + i] == '1') {
                parsed |= 1 << (7 - i);
            }
        }
        index += 8;
        str += parsed;
    }
    return str;
}

int Serializer::binaryToInt(std::vector<char>& vec, int index, int numberSize) {
    int decNum = 0;
    int base = 1;
    for (int i = numberSize; i >= 0; i--) {
        decNum += int(vec[i + index] - '0') * base;
        base *= 2;
    }
    return decNum;
}

//-------------------------------Massage Class-----------------------

Message::Message(std::string sender = "", std::string content = "", std::chrono::time_point<std::chrono::system_clock> timestamp = std::chrono::system_clock::now(), MessageType type = TEXT, MessageColor color = WHITE) {
    this->sender = sender;
    this->content = content;
    this->timestamp = timestamp;
    this->type = type;
    this->color = color;
}

std::string Message::getSender() {
    return sender;
}

std::string Message::getContent() {
    return content;
}

std::chrono::time_point<std::chrono::system_clock> Message::getTimeStamp() {
    return timestamp;
}

MessageType Message::getType() {
    return type;
}

MessageColor Message::getColor() {
    return color;
}


//-------------------------------Server Class-----------------------
Server::Server(int portNumber) {
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw runtime_error("WSAStartup failed!");
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        WSACleanup();
        throw runtime_error("Socket creation failed: " + WSAGetLastError());
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNumber);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(serverSocket);
        WSACleanup();
        throw runtime_error("Bind failed: " + WSAGetLastError());
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(serverSocket);
        WSACleanup();
        throw runtime_error("Listen failed: " + WSAGetLastError());
    }   

    FD_ZERO(&master);
    FD_SET(serverSocket, &master);
}

Server::~Server() {
    closesocket(serverSocket);
    WSACleanup();
}


bool Server::checkIfFile(Message &msg) {    
    return msg.getType() == FILEE;
}


bool Server::checkIfCommand(Message &msg) {        
    return msg.getType() == COMMAND;
}



void Server::reciveAndSendFile(fd_set master, SOCKET sendingSocket) {

    int bufferSize = 250;
    char* buffer = new char[bufferSize];
    
    int numberOfChuncks = 0;
    int lastChunckSize = 0;

    recv(sendingSocket, (char*)&numberOfChuncks, sizeof(numberOfChuncks), 0);
    recv(sendingSocket, (char*)&lastChunckSize, sizeof(lastChunckSize), 0);

    for (int i = 0; i < master.fd_count; i++) {
        SOCKET outSock = master.fd_array[i];
        if (outSock != serverSocket && outSock != sendingSocket) {
            send(outSock, (char*)&numberOfChuncks, sizeof(numberOfChuncks), 0);
            send(outSock, (char*)&lastChunckSize, sizeof(lastChunckSize), 0);                
        }
    }

    for (int i = 0; i < numberOfChuncks; i++) {
        recv(sendingSocket, buffer, bufferSize, 0);

        for (int j = 0; j < master.fd_count; j++) {
            SOCKET outSock = master.fd_array[j];
            if (outSock != serverSocket && outSock != sendingSocket) {
                send(outSock, buffer, bufferSize, 0);
            }                    
        }
                        
    }

    recv(sendingSocket, buffer, lastChunckSize, 0);
    for (int j = 0; j < master.fd_count; j++) {
        SOCKET outSock = master.fd_array[j];
        if (outSock != serverSocket && outSock != sendingSocket) {
            send(outSock, buffer, lastChunckSize, 0);
        }           
    }
}



bool Server::checkUser(std::string name) {

    for (int i = 0; i < clients.size(); i++) {
        if (strcmp(name.c_str(), clients[i].name.c_str()) == 0) {
            return true;
        }
    }
    return false;


}




void Server::handaleCommand(SOCKET sock, Message &msg1) {


    if (strcmp(msg1.getContent().c_str(), "/list") == 0) {
        std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();


        std::string senderName = "";
        std::string othersNames = "";
        int counter = 1;

        for (int i = 0; i < clients.size(); i++) {
            if (clients[i].sock != sock) {
                othersNames += counter + '0';
                othersNames += ". ";
                othersNames += clients[i].name;
                othersNames += '\n';
                counter++;
            }
            else {
                senderName += "* " + clients[i].name + " *" + '\n';
            }
        }

        senderName += othersNames;


        Message msg("Server", senderName, timestamp, TEXT);      
        sendMessage(sock, msg);


    }
    else if (strcmp(msg1.getContent().c_str(), "/quit") == 0) {

        string senderName = msg1.getSender();

        for (int i = 0; i < clients.size(); i++) {
            if (strcmp(clients[i].name.c_str(), senderName.c_str()) == 0) {
                clients.erase(clients.begin() + i);
            }
        }

        FD_CLR(sock, &master);
        closesocket(sock);
    }
    else {
        string sender = msg1.getSender();
        string content = msg1.getContent();
        MessageColor color = msg1.getColor();
        string reciver;
        string sendingContent;

        int index = 4;

        while (content[index] != ' ') {
            reciver += content[index];
            index++;
        }

        index++;

        for (int i = index; i < content.size(); i++) {
            sendingContent += content[i];
        }
        
        if (!checkUser(reciver)) {
            // send an error message to the sender
        }


        

        for (int i = 0; i < clients.size(); i++) {
            if (strcmp(reciver.c_str(), clients[i].name.c_str()) == 0) {
                std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
                Message msg3(sender, sendingContent, timestamp, TEXT, color);                
                sendMessage(clients[i].sock, msg3);

            }
            
        }


    }
    
    
}



void Server::runServer() {
    thread t3(&Server::acceptclients, this);
    thread t4(&Server::handelMessages, this);

    t3.join(); 
    t4.join();

}




void Server::acceptclients() {
    while (true) {

        fd_set copy = master;

        int socketCount = select(0, &copy, nullptr, nullptr, nullptr);
        for (int i = 0; i < socketCount; i++) {
            SOCKET sock = copy.fd_array[i];
            if (sock == serverSocket) {
                SOCKET client = accept(serverSocket, nullptr, nullptr);

                FD_SET(client, &master);

                std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();

                Message msg("Server", "Welcome to David's Chat!!!", timestamp, TEXT);
                sendMessage(client, msg);


                thread(&Server::valideteClientName, this, client).detach();
            }
        }
    }
}


void Server::valideteClientName(SOCKET client) {
    bool isValidName = false;

    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
    Message comfirNameMsg("Server", "1", timestamp, TEXT);

    Message uncomfirNameMsg("Server", "0", timestamp, TEXT);


    while (!isValidName) {




        Message msg2 = reciveMessage(client);
        string senderName = msg2.getSender();

        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            int counter = 0;

            for (int i = 0; i < clients.size(); i++) {
                if (strcmp(clients[i].name.c_str(), senderName.c_str()) != 0) {
                    counter++;
                }
            }

            if (counter == clients.size()) {
                NameAndSock client1;
                client1.name = msg2.getSender();
                client1.sock = client;
                clients.push_back(client1);
                isValidName = true;

                sendMessage(client, comfirNameMsg);
            }
            else {
                sendMessage(client, uncomfirNameMsg);
            }
        }
    }

}

void Server::handelMessages() {
    while (true) {
        fd_set copy = master;

        int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

        for (int i = 0; i < socketCount; i++) {
            SOCKET sock = copy.fd_array[i];
            if (sock != serverSocket) {
                Message msg;
                try {
                        msg = reciveMessage(sock);
                }
                catch (const std::runtime_error& e) {

                    for (int i = 0; i < clients.size(); i++) {
                        if (clients[i].sock == sock) {
                            clients.erase(clients.begin() + i);
                        }
                    }

                    FD_CLR(sock, &master);
                    closesocket(sock);
                    break;
                }
                if (checkIfCommand(msg)) {
                    handaleCommand(sock, msg);

                }
                else {

                    for (int i = 0; i < master.fd_count; i++) {
                        SOCKET outSock = master.fd_array[i];
                        if (outSock != serverSocket && outSock != sock) {
                            sendMessage(outSock, msg);

                        }
                    }

                    if (checkIfFile(msg)) {
                        reciveAndSendFile(master, sock);
                    }
                }
            }
           
        }

    }

}



/*

void Server::runServer() {
 

    while (true) {
        fd_set copy = master;

        int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

        for (int i = 0; i < socketCount; i++) {
            SOCKET sock = copy.fd_array[i];
            if (sock == serverSocket) {
                acceptNewClient();
            }
            else {
                Message msg;
                try{ 
                    msg = reciveMessage(sock);           
                }
                catch (const std::runtime_error& e) {                    

                    for (int i = 0; i < clients.size(); i++) {
                        if (clients[i].sock == sock) {
                            clients.erase(clients.begin() + i);
                        }
                    }

                    FD_CLR(sock, &master);
                    closesocket(sock);
                    break;
                }
                if (checkIfCommand(msg)) {
                    handaleCommand(sock, msg);
                   
                }
                else {

                    for (int i = 0; i < master.fd_count; i++) {
                        SOCKET outSock = master.fd_array[i];
                        if (outSock != serverSocket && outSock != sock) {
                            sendMessage(outSock, msg);

                        }
                    }

                    if (checkIfFile(msg)) {
                        reciveAndSendFile(master, sock);
                    }
                }

                
            }
        }

    }
}




void Server::valideteClientName(SOCKET client) {
    bool isValidName = false;

    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
    Message comfirNameMsg("Server", "1", timestamp, TEXT);

    Message uncomfirNameMsg("Server", "0", timestamp, TEXT);


    while (!isValidName) {




        Message msg2 = reciveMessage(client);
        string senderName = msg2.getSender();

        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            int counter = 0;

            for (int i = 0; i < clients.size(); i++) {
                if (strcmp(clients[i].name.c_str(), senderName.c_str()) != 0) {
                    counter++;
                }
            }

            if (counter == clients.size()) {
                NameAndSock client1;
                client1.name = msg2.getSender();
                client1.sock = client;
                clients.push_back(client1);
                isValidName = true;

                sendMessage(client, comfirNameMsg);

            }
            else {
                sendMessage(client, uncomfirNameMsg);

            }
        }
    }

}





void Server::acceptNewClient() {
    SOCKET client = accept(serverSocket, nullptr, nullptr);

    FD_SET(client, &master);

    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();

    Message msg("Server", "Welcome to David's Chat!!!", timestamp, TEXT);
    sendMessage(client, msg);


    thread(&Server::valideteClientName, this, client).detach();
    
}

*/






//-------------------------------Client Class-----------------------

Client::Client(std::string ipAddress, int portNumber) {
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw runtime_error("WSAStartup failed!");
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        WSACleanup();
        throw runtime_error("Socket creation failed: " + WSAGetLastError());
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNumber);
    inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(clientSocket);
        WSACleanup();
        throw runtime_error("Connection failed " + WSAGetLastError());
    }

    Message msg2 = reciveMessage(clientSocket);

    std::cout << "\n" << msg2.getContent() <<  endl;


    getSenderName();

    Message msg(senderName, "", msg2.getTimeStamp(), TEXT);
    sendMessage(clientSocket, msg);


    bool isValidName = false;


    while (!isValidName) {
        


        Message anserFromServerMsg = reciveMessage(clientSocket);
        string serverAnser = anserFromServerMsg.getContent();

        if (strcmp(serverAnser.c_str(), "0") == 0) {

            cout << "\nYour name is already taken" << endl;
            getSenderName();

            Message msg(senderName, "", anserFromServerMsg.getTimeStamp(), TEXT);
            sendMessage(clientSocket, msg);

        }
        else {
            isValidName = true;
        }
    }

    if (strcmp("Liran", senderName.c_str()) == 0) {
        cout << "Hello Yaroslav" << endl;
        color = PINCK;
    }
    else {
        getUserColor();
    }
    




    string input = "";

}

Client::~Client() {
    setColor(WHITE);
    closesocket(clientSocket);
    WSACleanup();
}

void Client::runClient() {
    thread t1(&Client::sendMsg, this);
    thread t2(&Client::receiveMsg, this);

    t1.join();
    t2.join();
}


void Client::getUserColor() {
    cout << "\nWhat color of text would you like?" << endl;
    cout << "Enter the number of the color that you want:" << endl;
    cout << "1. Blur, 2. Green, 3. Light Blue, 4. Red, 5. Pinck, 6. Yellow, 7. White" << endl;
    cout << "Color number: ";


    int intColor;
    cin >> intColor;

    if (!cin || (intColor < 1 || intColor > 7)) {
        cout << "\nYou entered a not valid number, Your color is white." << endl;
        color = WHITE;
    }
    else {
        switch (intColor)
        {
        case 1:
            color = BLUE;
            break;
        case 2:
            color = GREEN;
            break;
        case 3:
            color = LIGHT_BLUE;
            break;
        case 4:
            color = RED;
            break;
        case 5:
            color = PINCK;
            break;
        case 6:
            color = YELLOW;
            break;
        case 7:
            color = WHITE;
            break;
        default:
            color = WHITE;
            break;
        }
    }
}

void setColor(int textColor)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, textColor);
}



void Client::recvFile(Message &msg) {

    std::chrono::system_clock::time_point timestamp2 = msg.getTimeStamp();
    std::time_t time_t_timestamp2 = std::chrono::system_clock::to_time_t(timestamp2);

    std::cout << "\r" << std::put_time(std::localtime(&time_t_timestamp2), "%H:%M:%S") << std::endl;
    std::cout << msg.getSender() << ": ";
    std::cout << "sending a file" << "\n" << std::endl;

    string fileName = "copy_";
    fileName += msg.getContent();

    ofstream file(fileName, ios::binary | ios::app);


    int bufferSize = 250;

    char* buffer = new char[bufferSize];

    int numberOfChuncks = 0;
    int lastChunckSize = 0;



    recv(clientSocket, (char*)&numberOfChuncks, sizeof(numberOfChuncks), 0);
    recv(clientSocket, (char*)&lastChunckSize, sizeof(lastChunckSize), 0);

            

    for (int i = 0; i < numberOfChuncks; i++) {
        recv(clientSocket, buffer, bufferSize, 0);
        file.write(buffer, bufferSize);
                        
    }

    recv(clientSocket, buffer, lastChunckSize, 0);
    file.write(buffer, lastChunckSize);
                
                
    file.close();  
}



bool isQuit = false;

void Client::receiveMsg() {
  

    while (true) {

        
        Message msg2;
        
        try {
            msg2 = reciveMessage(clientSocket);
        }
        catch (const std::runtime_error& e) {
            cout << e.what()  << "Server is down, press any key to contine" << endl;
        }

        
        

        if (isQuit) {
            break;
        }

        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            for (int i = 0; i < input.size() + senderName.size() + 2; i++) {
                cout << '\b';
                cout << ' ';
                cout << '\b';
            }
                
            std::cout << "\n";
                

            if (msg2.getType() == FILEE) {
                recvFile(msg2);
            }
            else {
                              

                if (strcmp(msg2.getSender().c_str(), "Server") == 0) {
                    std::cout << msg2.getContent() << "\n" << std::endl;
                }
                else {
                    setColor(msg2.getColor());
                    std::chrono::system_clock::time_point timestamp2 = msg2.getTimeStamp();
                    std::time_t time_t_timestamp2 = std::chrono::system_clock::to_time_t(timestamp2);

                    std::cout << "\r" << std::put_time(std::localtime(&time_t_timestamp2), "%H:%M:%S") << std::endl;
                    std::cout << msg2.getSender() << ": ";
                    std::cout << msg2.getContent() << "\n" << std::endl;
                    setColor(color);
                }
                

            }  
            
            cout << senderName + ": " << input;
        }
        
    }
}



std::string Client::getFileName(std::string filePath) {
    
    
    int index = filePath.size() - 1;
    
    string revFileName;
    string fileName;
    while (filePath[index] != '\\' && filePath[index] != ' ') {
        
        revFileName += filePath[index];
        index--;
    }


    for (int i = revFileName.size() - 1; i >= 0; i--) {
        fileName += revFileName[i];
    
    }


    return fileName;

}





 





void Client::sendFile() {

    string filePath;
    for (int i = 4; i < input.size(); i++) {
        filePath += input[i];
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        cout << "\nFaild to open the input file" << endl;
        return;
    }

    string fileName =  getFileName(filePath);
        


    // Send a meesage to the server that a file is comming
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
    Message msg(senderName, fileName, timestamp,  FILEE);    
    sendMessage(clientSocket, msg);



    // send the file

    int bufferSize = 250;

    char* buffer = new char[bufferSize];

    std::streampos fileSize = 0;
    

    file.seekg(0, ios::end);
    fileSize = file.tellg();
    file.seekg(0, ios::beg);


    int numberOfChuncks = fileSize / bufferSize;
    int lastChunkSize = fileSize % bufferSize;

    send(clientSocket, (char*)&numberOfChuncks, sizeof(numberOfChuncks), 0);
    send(clientSocket, (char*)&lastChunkSize, sizeof(lastChunkSize), 0);


    for (int i = 0; i < numberOfChuncks; i++) {
        file.read(buffer, bufferSize);
        send(clientSocket, buffer, bufferSize, 0);
    }

    file.read(buffer, lastChunkSize);
    send(clientSocket, buffer, lastChunkSize, 0);


    file.close();
}






bool Client::checkIfCommad() {
    
    if (strcmp(input.c_str(), "/list") == 0) {
        return true;
    }
    else if (input[0] == '/' && input[1] == 'p' && input[2] == 'm') {
        return true;
    }
    else if (strcmp(input.c_str(), "/quit") == 0) {
        isQuit = true;
        return true;
    }
    else if (strcmp(input.c_str(), "/help") == 0) {
        return true;
    }
    else {
        return false;
    }
} 


void Client::handelCommand() {
    if (strcmp(input.c_str(), "/help") == 0) {
        cout << "\n";
        showUserInstructions();
    }
    else {
        std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();

        Message msg1(senderName, input, timestamp, COMMAND, color);       
        sendMessage(clientSocket, msg1);

    }
}






void Client::sendMsg() {
    while (true) {
        if (isQuit) {
            break;
        }
        setColor(color);
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            cout << "\n" + senderName + ": ";
        }

        getInput();

        if (strcmp(input.c_str(), "") == 0) {
            continue;
        }

        if (input[0] == '-' && input[1] == 'S') {
            sendFile();
        }
        else if (checkIfCommad()) {
                
            handelCommand();
        }
        else {
            std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();

            Message msg1(senderName, input, timestamp, TEXT, color);            
            sendMessage(clientSocket, msg1);
        }
        
    }
}





void Client::getInput() {
    bool running = true;
    bool toLongInput = false;
    input = "";
    char ch;
    while (running) {
        ch = _getch();

        if (isQuit) {
            break;
        }

        if (ch == '\r') {
            running = false;
        }
        else if (ch == '\b') {
            if (input == "") {
                continue;
            }
            cout << '\b';
            cout << ' ';
            cout << '\b';
            input.pop_back();
        }
        else if (input.size() < (MAX_BUF_SIZE - 1)) {
            input += ch;
            cout << ch;
        }
        else {
            if (!toLongInput) {
                cout << "\nYou string is too long, Press Enter to continue.";
                toLongInput = true;
            }

        }
    }
    if (toLongInput) {
        input = "";
    }
}

void Client::getSenderName() {
    cout << "Enter your name: ";
    cin >> senderName;
}



void Client::showUserInstructions() {
    cout << "\n";
    cout << "Instructions for using the chat application:\n";
    cout << "--------------------------------------------\n";
    cout << "1. To send a message: Type your message and press Enter.\n";
    cout << "2. To see who is in the chat: Type /list and press Enter.\n";
    cout << "3. To leave the chat: Type /quit and press Enter.\n";
    cout << "4. To send a private message: Type /pm <username> <message> and press Enter.\n";
    cout << "   Example: /pm John Hi, how are you?\n";
    cout << "5. To send a file to everyone: Type -SF <file path> and press Enter.\n";
    cout << "   Example: -SF C:\\Users\\YourName\\Documents\\file.txt\n";
    cout << "6. To see another time those instructions: Type /help and press Enter.\n";
    cout << "--------------------------------------------\n\n";
}





bool isArgcValid(int argc) {
    if (argc == 1) {
        cout << "Please insert argument." << endl;
        
    }
    else if (argc == 2) {
        cout << "Too few arguments." << endl;
    }
    else if (argc > 3) {
        cout << "Too many arguments." << endl;
    }
    else {
        return true;
    }
    return false;
}




void sendMessage(SOCKET socket, Message &msg) {
    std::vector<char> vec = Serializer::serialize(msg);
    int vectorSize = vec.size();
    send(socket, (char*)&vectorSize, sizeof(vectorSize), 0);
    send(socket, vec.data(), vectorSize, 0);
}

Message reciveMessage(SOCKET socket) {
    int vectorSize = 0;
    int num = recv(socket, (char*)&vectorSize, sizeof(vectorSize), 0);
    if (num == 0) {
        std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
        Message faildMsg("", "-1", timestamp, TEXT);
        return faildMsg;
    }
    if (num == -1) {
        isQuit = true;
        throw runtime_error("\n");
    }
    
    std::vector<char> vec(vectorSize);
    recv(socket, vec.data(), vectorSize, 0);


    Message msg = Serializer::deserialize(vec);
    return msg;
}


int main(int argc, char* argv[]) {
    if (!isArgcValid(argc)) {
        return 0;
    }
    
    if (strcmp(argv[1], "-S") == 0) {
        Server server(atoi(argv[2]));
        server.runServer();
    }
    else {
        std::string s(argv[1]);     
        try {

            Client client(s, atoi(argv[2]));
            client.showUserInstructions();
            client.runClient();

            client.~Client();
        }
        catch (const std::runtime_error& e) {
            cout << e.what() << endl;
        }

    }

    

   
    

    
    return 0;
}

