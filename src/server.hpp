#ifndef HW3_H
#define HW3_H
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <time.h>
#include <set>
#include <cstring>

using namespace std;

class User{
public:
    User(){
        loginClientId = -1;
        banned = false;
        chatRoomPort = -1;
        chatRoomVersion = -1;
        violationCount = 0;
    }

    User(string username, string password)
        :username(username), password(password), loginClientId(-1),
         banned(false), chatRoomPort(-1), chatRoomVersion(-1),
         violationCount(0){}

    User(char* username, char* password)
        :username(username), password(password), loginClientId(-1),
         banned(false), chatRoomPort(-1), chatRoomVersion(-1),
         violationCount(0){}
    
    string username;
    string password;
    int loginClientId;
    bool banned;
    int chatRoomPort; // 0 for unset
    int chatRoomVersion; // 0 for unset
    int violationCount;
};

class Comment{
public:
    Comment(){}
    Comment(string user, string comment){
        this->user = user;
        this->comment = comment;
    }
    string user;
    string comment;
};

class Post{
public:
    static int postCount;
    Post(){
        this->date = updateTime();
        this->postSN = postCount++;
    }
    Post(string title, string content, string author, int boardId){
        this->title = title;
        this->content = content;
        this->author = author;
        this->date = updateTime();
        this->postSN = postCount++;
        this->boardId = boardId;
    }

    string updateTime(){
        time_t     now = time(0);
        struct tm  tstruct;
        char       buf[80];
        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%m/%d", &tstruct);
        return string(buf);
    }

    string title;
    string content;
    string author;
    string date;
    int postSN;
    int boardId;
    vector<Comment> comments;
};

class Board{
public:
    static int boardCount;
    Board(){boardCount++;}
    Board(string name, string moderator){
        this->name = name;
        this->moderator = moderator;
        index = boardCount;
        boardCount++;
    }
    int index;
    string name;
    vector<int> posts; // post indices
    string moderator;
};

int client[FD_SETSIZE];
vector<string> clientOnlineUser(FD_SETSIZE);

map<string,User*> users;
vector<Board*> boards;
vector<Post*> posts;
map<string,int> boardStrId;
set<string> chatRoomUsers;
vector<string> chatHistory;

int Post::postCount = 0;
int Board::boardCount = 0;

#endif