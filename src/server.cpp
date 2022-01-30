#include "server.hpp"
#include "message.hpp"
#include "base64.hpp"
#define LISTENQ 1024
#define MAXLINE 4096

int handleCommand(int sockfd, int clientID, stringstream& ss);
void registerUser(int sockfd, stringstream& ss);
void sendline(int sockfd, const char* content);
void showUsage(int sockfd, const char* msg);
void userLogin(int sockfd, stringstream& ss, int clientID);
void userLogout(int sockfd, stringstream& ss, int clientID);
bool handleExit(int sockfd, stringstream& ss, int clientID);
void createBoard(int sockfd, stringstream& ss, int clientID);
void listBoard(int sockfd, stringstream& ss);
void createPost(int sockfd, stringstream& ss, int clientID);
void listPost(int sockfd, stringstream& ss);
void readPost(int sockfd, stringstream& ss);
void deletePost(int sockfd, stringstream& ss, int clientId);
void updatePost(int sockfd, stringstream& ss, int clientId);
void commentPost(int sockfd, stringstream& ss, int clientId);
int handleClient(int sockfd, int clientID){
    char buff[MAXLINE];
    FILE* fpin = fdopen(sockfd, "r+");
    setvbuf(fpin, NULL, _IONBF, 0);
    if(fgets(buff, MAXLINE, fpin) == NULL){
        return 1;
    }
    stringstream ss(buff);
    return handleCommand(sockfd, clientID, ss);
}
int getBoardId(string boardName);

// hw3 features
void enterChatRoom(int sockfd, stringstream& ss, int clientId);


int main(int argc, char **argv){
    if(argc != 2) {
        cout << "usage: ./server [port number]" << endl;
        return 1;
    }
    int SERV_PORT = atoi(argv[1]);

    int i, maxi, maxfd, listenfd, connfd, sockfd, udpfd;
    int nready, client[FD_SETSIZE];
    ssize_t n;
    fd_set rset, allset;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int enable = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    
    bind(listenfd, (struct  sockaddr*) &servaddr, sizeof(servaddr));
    listen(listenfd, LISTENQ);

    // UDP socket
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    bind(udpfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
    listen(udpfd, LISTENQ);

    maxfd = max(listenfd, udpfd);
    maxi = -1;

    for (i = 0; i < FD_SETSIZE ; i++)
        client[i] = -1;
    
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    FD_SET(udpfd, &allset);

    for ( ; ; ) {
        rset = allset; // structure assignment
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);
        // new client connection
        if(FD_ISSET(listenfd, &rset)){
            clilen = sizeof(cliaddr);
            connfd = accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);

            for(i = 0 ; i < FD_SETSIZE ; i++){
                if(client[i] < 0){
                    client[i] = connfd;
                    break;
                }
            }
            if (i == FD_SETSIZE){
                exit(1);
            }
            sendline(connfd, "********************************\n** Welcome to the BBS server. **\n********************************\n% ");
            FD_SET(connfd, &allset); // add new descriptor to set (to watch)
            if(connfd > maxfd) maxfd = connfd;
            if(i > maxi) maxi = i; // max index in client[] array
            if(--nready <= 0) continue;
        }

        // check for UDP packets
        if(FD_ISSET(udpfd, &rset)){
            unsigned char packet[MAXLINE];
            clilen = sizeof(cliaddr);
            n = recvfrom(udpfd, packet, MAXLINE, 0, (sockaddr *) &cliaddr, &clilen);
            // get port
            // decode UDP packet
            int version = 0;
            struct Header *recvHeader = (struct Header *) packet;
            char name[MAXLINE];
            char msg[MAXLINE];

            if(recvHeader->version == 1){
                struct BodyMf1 *nameBody = (struct BodyMf1 *) (packet + sizeof(struct Header));
                uint16_t nameLen = ntohs(nameBody->len);
                struct BodyMf1 *msgBody = (struct BodyMf1 *) (packet + sizeof(struct Header) + sizeof(struct BodyMf1) + nameLen);    
                uint16_t msgLen = ntohs(msgBody->len);
                memcpy(name, nameBody->data, nameLen);
                name[nameLen] = 0;
                memcpy(msg, msgBody->data, msgLen);
                msg[msgLen] = 0;
            }
            // version 2
            else{
                char encodedName[MAXLINE];
                char encodedMsg[MAXLINE];
                char *bodyPtr = (char *) (packet + sizeof(struct Header));
                unsigned int i = 0;
                while(*bodyPtr!='\n'){
                    encodedName[i++] = *bodyPtr;
                    bodyPtr++;
                }
                encodedName[i] = 0;
                bodyPtr++;
                i = 0;
                while(*bodyPtr!='\n'){
                    encodedMsg[i++] = *bodyPtr;
                    bodyPtr++;
                }
                encodedMsg[i] = 0;

                // decode message
                strcpy(name, base64Decode(encodedName).c_str());
                strcpy(msg, base64Decode(encodedMsg).c_str());
            }

            // filter messages
            string filteredMsg = string(msg);
            string user = string(name);
            bool violated = filterMsg(filteredMsg);
            // check if violated
            if(violated){

                User* vioUser = users[user];
                vioUser->violationCount += 1;
                if(vioUser->violationCount >= 3){
                    vioUser->banned = true;
                    int clientId = vioUser->loginClientId;
                    int sockfd = client[clientId];
                    stringstream ss;
                    userLogout(sockfd, ss, clientId); // also exits chatroom
                    sendline(sockfd, "% ");
                }
            }

            // save messages
            string saveMsg;
            saveMsg += string(name) + ":";
            saveMsg += filteredMsg + "\n";
            chatHistory.push_back(saveMsg);

            uint16_t nameLen = (uint16_t) strlen(name);
            uint16_t msgLen = (uint16_t) filteredMsg.size();
            char encodedName[MAXLINE];
            char encodedMsg[MAXLINE];
            strcpy(encodedName, base64Encode(name).c_str());
            strcpy(encodedMsg, base64Encode(filteredMsg).c_str());
            uint16_t encodedNameLen = (uint16_t) strlen(encodedName);
            uint16_t encodedMsgLen = (uint16_t) strlen(encodedMsg);
            // send messages
            for(auto& username: chatRoomUsers){
                User* onlineUser = users[username];
                int userPort = onlineUser->chatRoomPort;
                int userVersion = onlineUser->chatRoomVersion;
                cliaddr.sin_port = htons(userPort);
                char sendBuf[MAXLINE];
                if(userVersion==1) {
                    struct Header *sendHeader = (struct Header *) sendBuf;
                    struct BodyMf1 *nameBody = (struct BodyMf1 *) (sendBuf + sizeof(struct Header));
                    struct BodyMf1 *msgBody = (struct BodyMf1 *) (sendBuf + sizeof(struct Header) + sizeof(struct BodyMf1) + nameLen);
                    sendHeader->flag = 0x01;
                    sendHeader->version = 0x01;
                    nameBody->len = htons(nameLen);
                    memcpy(nameBody->data, name, nameLen);
                    msgBody->len = htons(msgLen);
                    memcpy(msgBody->data, filteredMsg.c_str(), msgLen);
                    ssize_t totalLen = sizeof(struct Header) + sizeof(struct BodyMf1)*2 + nameLen + msgLen;
                    sendto(udpfd, sendBuf, totalLen, 0, (sockaddr *) &cliaddr, clilen);
                }
                // version 2
                else {
                    sprintf(sendBuf, "\x01\x02%s\n%s\n", encodedName, encodedMsg);
                    ssize_t totalLen = 4 + encodedNameLen + encodedMsgLen;
                    sendto(udpfd, sendBuf, totalLen, 0, (sockaddr *) &cliaddr, clilen);
                }
            }
            // sendto(udpfd, packet, n, 0, (sockaddr *) &cliaddr, clilen);
        }

        // check all clients for data
        for(i = 0; i <= maxi; i++){
            if((sockfd = client[i]) < 0) // not connected
                continue;

            if(FD_ISSET(sockfd, &rset)) {
                int stat = handleClient(sockfd, i);
                if(stat==1){
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                    if(!clientOnlineUser[i].empty()){
                        users[clientOnlineUser[i]]->loginClientId = -1;
                    }
                    clientOnlineUser[i].clear();
                }
                if (--nready <= 0) break; // finished reading all descriptors
                return 1;
            }
        }
    }
    return 0;
}

void sendline(int sockfd, const char* content){
    FILE* fpout = fdopen(sockfd, "w+");
    setvbuf(fpout, NULL, _IONBF, 0);
    fputs(content, fpout);
}

void showUsage(int sockfd, const char* msg){
    string usage = "Usage: ";
    usage += msg;
    usage += "\n";
    sendline(sockfd, usage.c_str());
}

// return 1 if user exits
int handleCommand(int sockfd, int clientID, stringstream& ss){
    string command;
    ss >> command;
    if(command=="register"){
        registerUser(sockfd, ss);
    }
    else if (command=="login"){
        userLogin(sockfd, ss, clientID);
    }
    else if (command=="logout"){
        userLogout(sockfd, ss, clientID);
    }
    else if (command=="exit"){
        if(handleExit(sockfd, ss, clientID)) return 1;
    }
    else if (command=="create-board"){
        createBoard(sockfd, ss, clientID);
    }
    else if (command=="list-board"){
        listBoard(sockfd, ss);
    }
    else if (command=="create-post"){
        createPost(sockfd, ss, clientID);
    }
    else if (command=="list-post"){
        listPost(sockfd, ss);
    }
    else if (command=="read"){
        readPost(sockfd, ss);
    }
    else if (command=="delete-post"){
        deletePost(sockfd, ss, clientID);
    }
    else if (command=="update-post"){
        updatePost(sockfd, ss, clientID);
    }
    else if (command=="comment"){
        commentPost(sockfd, ss, clientID);
    }
    else if (command=="enter-chat-room"){
        enterChatRoom(sockfd, ss, clientID);
    }
    sendline(sockfd, "% ");
    return 0;
}

void registerUser(int sockfd, stringstream& ss){
    string s;
    vector<string> args;
    while(ss>>s){
        args.push_back(s);
    }
    if(args.size() != 2){
        showUsage(sockfd, "register <username> <password>");
        return;
    }
    if(users.find(args[0]) != users.end()){
        sendline(sockfd, "Username is already used.\n");
        return;
    }
    users.insert(make_pair(string(args[0]), new User(args[0], args[1])));
    sendline(sockfd, "Register successfully.\n");
}

void userLogin(int sockfd, stringstream& ss, int clientID){
    string s;
    vector<string> args;
    while(ss>>s){
        args.push_back(s);
    }
    if(args.size() != 2){
        showUsage(sockfd, "login <username> <password>");
        return;
    }
    // client has already logged in
    if(!clientOnlineUser[clientID].empty()){
        sendline(sockfd, "Please logout first.\n");
        return;
    }
    // user is found
    if(users.find(args[0])!=users.end()){
        if(users[args[0]]->loginClientId!=-1){
            sendline(sockfd, "Please logout first.\n");
            return;
        }
        if(users[args[0]]->banned){
            char msg[MAXLINE];
            snprintf(msg, MAXLINE, "We don't welcome %s!\n", users[args[0]]->username.c_str());
            sendline(sockfd, msg);
            return;
        }
    }
    if(users.find(args[0]) == users.end()){
        sendline(sockfd, "Login failed.\n");
        return;
    }
    User* user = users[args[0]];
    if(user->password!=args[1]){
        sendline(sockfd, "Login failed.\n");
        return;
    }
    
    clientOnlineUser[clientID] = user->username;
    char msg[MAXLINE];
    snprintf(msg, MAXLINE, "Welcome, %s.\n", clientOnlineUser[clientID].c_str());
    sendline(sockfd, msg);
    user->loginClientId = clientID; // login client
}

void userLogout(int sockfd, stringstream& ss, int clientID){
    string s;
    vector<string> args;
    while(ss>>s){
        args.push_back(s);
    }
    if(args.size() != 0){
        showUsage(sockfd, "logout");
        return;
    }
    if(clientOnlineUser[clientID].empty()){
        sendline(sockfd, "Please login first.\n");
        return;
    }
    
    char msg[MAXLINE];
    snprintf(msg, MAXLINE, "Bye, %s.\n", clientOnlineUser[clientID].c_str());
    sendline(sockfd, msg);
    if(!clientOnlineUser[clientID].empty()){
        string currentUser = clientOnlineUser[clientID];
        users[currentUser]->loginClientId = -1;
        if(users[currentUser]->chatRoomVersion != -1){
            chatRoomUsers.erase(currentUser);
        }
        users[currentUser]->chatRoomPort = -1;
        users[currentUser]->chatRoomVersion = -1;
    }
    clientOnlineUser[clientID].clear();
}

bool handleExit(int sockfd, stringstream& ss, int clientID){
    string s;
    vector<string> args;
    while(ss>>s){
        args.push_back(s);
    }
    if(args.size() != 0){
        showUsage(sockfd, "exit");
        return false;
    }
    if(clientOnlineUser[clientID].empty()){
        return true;
    }
    userLogout(sockfd, ss, clientID);
    return true;
}

void createBoard(int sockfd, stringstream& ss, int clientID){
    vector<string> args;
    string s;
    while(ss >> s){
        args.push_back(s);
    }
    if(args.size()!=1){
        showUsage(sockfd, "create-board <name>");
        return;
    }
    string user;
    if( (user = clientOnlineUser[clientID]).empty() ){
        sendline(sockfd, "Please login first.\n");
        return;
    }
    if(boardStrId.find(args[0])!=boardStrId.end()){
        sendline(sockfd, "Board already exists.\n");
        return;
    }
    boards.push_back(new Board(args[0], user));
    boardStrId[args[0]] = boards[boards.size()-1]->index;
    sendline(sockfd, "Create board successfully.\n");
}

void listBoard(int sockfd, stringstream& ss){
    string s;
    if(ss >> s){
        showUsage(sockfd, "list-board");
        return;
    }
    stringstream allContent;
    allContent << "Index Name Moderator" << endl;
    for(unsigned int i = 0 ; i < boards.size() ; i++){
        allContent << (boards[i] -> index) + 1 << " " << 
        (boards[i]->name) << " " << 
        (boards[i]->moderator) << endl;
    }
    string content = allContent.str();
    sendline(sockfd, content.c_str());
    return;
}

void createPost(int sockfd, stringstream& ss, int clientID){
    vector<string> args;
    string s;
    int titleIdx = 0;
    int contentIdx = 0;
    int i = 0;
    while(ss >> s){
        args.push_back(s);
        if(s == "--title") titleIdx=i;
        if(s == "--content") contentIdx=i;
        i++;
    }
    if(abs(titleIdx-contentIdx)<=1||titleIdx==0||contentIdx==0||titleIdx==args.size()-1||contentIdx==args.size()-1||args.size()<5){
        showUsage(sockfd, "create-post <board-name> --title <title> --content <content>");
        return;
    }
    if(clientOnlineUser[clientID].empty()){
        sendline(sockfd, "Please login first.\n");
        return;
    }
    
    string boardName = args[0];
    int boardId;
    if( (boardId = getBoardId(boardName)) == -1 ){
        sendline(sockfd, "Board does not exist.\n");
        return;
    }
    stringstream title;
    stringstream content;
    int mode = 0;
    bool titleBegin = true;
    bool contentBegin = true;

    for(int i = 1 ; i < args.size() ; i++){
        if(args[i]=="--title"){
            mode = 0;
            continue;
        }
        if(args[i]=="--content"){
            mode = 1;
            continue;
        }
        if(mode == 0){
            if(titleBegin){
                titleBegin = false;
                title << args[i];
                continue;
            }
            title << " " << args[i];
        }
        if(mode == 1){
            if(contentBegin){
                contentBegin = false;
                content << args[i];
                continue;
            }
            content << " " << args[i];
        }
    }

    string contentStr = content.str();
    string token = "<br>";
    int pos = contentStr.find(token);
    while(pos != string::npos){
        contentStr.replace(pos, 4, "\n");
        pos = contentStr.find(token);
    }
    Post* newPost = new Post(title.str(), contentStr, clientOnlineUser[clientID], boardId);
    int postId = newPost->postSN;
    posts.push_back(newPost);
    boards[boardId]->posts.push_back(postId);
    sendline(sockfd, "Create post successfully.\n");
}

int getBoardId(string boardName){
    if( boardStrId.find(boardName) == boardStrId.end() ){
        return -1;
    }
    return boardStrId[boardName];
}

void listPost(int sockfd, stringstream& ss){
    vector<string> args;
    string s;
    while(ss >> s){
        args.push_back(s);
    }
    if(args.size()!=1){
        showUsage(sockfd, "list-post <board-name>");
        return;
    }
    int boardId;
    if((boardId = getBoardId(args[0]))==-1){
        sendline(sockfd, "Board does not exist.\n");
        return;
    }
    string content = "S/N     Title     Author     Date\n";
    for(int i = 0 ; i < boards[boardId]->posts.size() ; i++){
        int postIdx = boards[boardId]->posts[i];
        if(posts[postIdx]==NULL){
            continue;
        }
        Post* curPost = posts[postIdx];
        char buff[MAXLINE];
        snprintf(buff, MAXLINE, "%d %s %s %s\n", curPost->postSN + 1, curPost->title.c_str(), curPost->author.c_str(), curPost->date.c_str());
        content += buff;
    }
    sendline(sockfd, content.c_str());
}

void readPost(int sockfd, stringstream& ss){
    vector<string> args;
    string s;
    while(ss >> s){
        args.push_back(s);
    }
    if(args.size() != 1){
        showUsage(sockfd, "read <post-S/N>");
        return;
    }

    int postId;
    try {
        postId = stoi(args[0]);
    } catch (...){
        sendline(sockfd, "Post does not exist.\n");
        return;
    }
    postId -= 1;
    if(postId >= posts.size()){
        sendline(sockfd, "Post does not exist.\n");
        return;
    }
    if(postId < 0){
        showUsage(sockfd, "read <post-S/N>");
        return;
    }
    if(posts[postId] == NULL){
        sendline(sockfd, "Post does not exist.\n");
        return;
    }
    Post* viewPost = posts[postId];
    string postContent;
    postContent += "Author: " + viewPost->author + "\n";
    postContent += "Title: " + viewPost->title + "\n";
    postContent += "Date: " + viewPost->date + "\n";
    postContent += "--\n";
    postContent += viewPost->content + "\n";
    postContent += "--\n";
    for(int i = 0 ; i < viewPost->comments.size(); i++){
        string comment = viewPost->comments[i].comment;
        string user = viewPost->comments[i].user;
        char commentBuff[MAXLINE];
        snprintf(commentBuff, MAXLINE, "%s: %s\n", user.c_str(), comment.c_str());
        postContent += commentBuff;
    }
    sendline(sockfd, postContent.c_str());
}

void deletePost(int sockfd, stringstream& ss, int clientId){
    string s;
    vector<string> args;
    while(ss >> s){
        args.push_back(s);
    }
    if(args.size() != 1){
        showUsage(sockfd, "delete-post <post-S/N>");
        return;
    }
    string username = clientOnlineUser[clientId];
    if(username.empty()){
        sendline(sockfd, "Please login first.\n");
        return;
    }
    int postId;
    try{
        postId = stoi(args[0]);
    }
    catch(...){
        sendline(sockfd, "Post does not exist.\n");
        return;
    }
    postId -= 1;
    if(postId >= posts.size()){
        sendline(sockfd, "Post does not exist.\n");
        return;
    }
    if(postId < 0){
        showUsage(sockfd, "delete-post <post-S/N>");
        return;
    }
    if(posts[postId] == NULL){
        sendline(sockfd, "Post does not exist.\n");
        return;
    }
    Post* curPost = posts[postId];
    if(curPost->author != username){
        sendline(sockfd, "Not the post owner.\n");
        return;
    }

    posts[postId] = NULL;
    delete curPost;
    sendline(sockfd, "Delete successfully.\n");
}

void updatePost(int sockfd, stringstream& ss, int clientId){
    string s;
    vector<string> args;
    while(ss >> s){
        args.push_back(s);
    }
    if(args.size() < 3){
        showUsage(sockfd, "update-post <post-S/N> --title/content <new>");
        return;
    }
    if(args[1]!="--title"&&args[1]!="--content"){
        showUsage(sockfd, "update-post <post-S/N> --title/content <new>");
        return;
    }
    string username = clientOnlineUser[clientId];
    if(username.empty()){
        sendline(sockfd, "Please login first.\n");
        return;
    }
    int postId;
    try{
        postId = stoi(args[0]);
    }
    catch(...){
        sendline(sockfd, "Post does not exist.\n");
        return;
    }
    postId -= 1;
    if(postId < 0){
        showUsage(sockfd, "update-post <post-S/N> --title/content <new>");
        return;
    }
    if(postId >= posts.size() || posts[postId] == NULL){
        sendline(sockfd, "Post does not exist.\n");
        return;
    }
    Post* curPost = posts[postId];
    if(curPost->author != username){
        sendline(sockfd, "Not the post owner.\n");
        return;
    }

    stringstream replacement;
    int mode = 0;
    bool begin = true;
    if(args[1]=="--content"){
        mode = 1;
    }
    for(int i = 2 ; i < args.size() ; i++){
        if(begin){
            begin = false;
            replacement << args[i];
            continue;
        }
        replacement << " " << args[i];
    }
    string replacementStr = replacement.str();
    if(mode){ // is content
        string token = "<br>";
        int pos = replacementStr.find(token);
        while(pos != string::npos){
            replacementStr.replace(pos, 4, "\n");
            pos = replacementStr.find(token);
        }
    }

    if(mode){ // content
        curPost->content = replacementStr;
    }
    else{
        curPost->title = replacementStr;
    }
    sendline(sockfd, "Update successfully.\n");
}

void commentPost(int sockfd, stringstream& ss, int clientId){
    string s;
    vector<string> args;
    while(ss >> s){
        args.push_back(s);
    }
    if(args.size()<2){
        showUsage(sockfd, "comment <post-S/N> <comment>");
        return;
    }
    if(clientOnlineUser[clientId].empty()){
        sendline(sockfd, "Please login first.\n");
        return;
    }
    string user = clientOnlineUser[clientId];
    int postId;
    try{
        postId = stoi(args[0]);
    }
    catch(...){
        sendline(sockfd, "Post does not exist.\n");
        return;
    }
    postId -= 1;
    if(postId >= posts.size()){
        sendline(sockfd, "Post does not exist.\n");
        return;
    }
    if(postId < 0){
        showUsage(sockfd, "comment <post-S/N> <comment>");
        return;
    }
    if(posts[postId] == NULL){
        sendline(sockfd, "Post does not exist.\n");
        return;
    }
    Post* curPost = posts[postId];
    stringstream comment;

    for(int i = 1 ; i < args.size() ; i++){
        if(i == 1){
            comment << args[i];
            continue;
        }
        comment << " " << args[i];
    }
    curPost -> comments.push_back(Comment(user,comment.str()));
    sendline(sockfd, "Comment successfully.\n");
}

void enterChatRoom(int sockfd, stringstream& ss, int clientId){
    string s;
    vector<string> args;
    while(ss >> s){
        args.push_back(s);
    }
    if(args.size() != 2){
        showUsage(sockfd, "enter-chat-room <port> <version>");
        return;
    }
    int port;
    int version;
    try {
        port = stoi(args[0]);
    } catch (...){
        char msg[MAXLINE];
        snprintf(msg, MAXLINE, "Port %s is not valid.\n", args[0].c_str());
        sendline(sockfd, msg);
        return;
    }
    if(!(port>=1&&port<=65535)){
        char msg[MAXLINE];
        snprintf(msg, MAXLINE, "Port %s is not valid.\n", args[0].c_str());
        sendline(sockfd, msg);
        return;
    }

    try {
        version = stoi(args[1]);
    } catch (...){
        char msg[MAXLINE];
        snprintf(msg, MAXLINE, "Version %s is not supported.\n", args[1].c_str());
        sendline(sockfd, msg);
        return;
    }
    if(!(version == 1 || version == 2)){
        char msg[MAXLINE];
        snprintf(msg, MAXLINE, "Version %s is not supported.\n", args[1].c_str());
        sendline(sockfd, msg);
        return;
    }

    if(clientOnlineUser[clientId].empty()){
        sendline(sockfd, "Please login first.\n");
        return;
    }

    string curUser = clientOnlineUser[clientId];
    
    // check whether user is already in chatroom, otherwise update current chat room online list
    if(users[curUser]->chatRoomVersion == -1){
        chatRoomUsers.insert(curUser);
    }
    users[curUser]->chatRoomPort = port;
    users[curUser]->chatRoomVersion = version;

    char msg[MAXLINE];
    snprintf(msg, MAXLINE, "Welcome to public chat room.\nPort:%d\nVersion:%d\n", port, version);
    string res(msg);
    for(string& s: chatHistory) res += s;
    sendline(sockfd, res.c_str());
}
