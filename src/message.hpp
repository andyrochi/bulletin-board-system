#ifndef HW3_MESSAGE_H
#define HW3_MESSAGE_H

#include <utility>

struct Header {
    unsigned char flag;
    unsigned char version;
    unsigned char payload[0];
} __attribute__((packed));

struct BodyMf1 {
    unsigned short len;
    unsigned char data[0];
} __attribute__((packed));

vector<pair<string,string>> filterList = {
    {"how", "***"},
    {"you", "***"},
    {"or", "**"},
    {"pek0", "****"},
    {"tea", "***"},
    {"ha", "**"},
    {"kon", "***"},
    {"pain", "****"},
    {"Starburst Stream", "****************"}
};

// filters message passed, return true if violated rule
bool filterMsg(string& message){
    bool violation = false;
    for(auto& filter: filterList){
        string token = filter.first;
        int pos = message.find(token);
        while(pos != string::npos){
            violation = true;
            message.replace(pos, filter.first.size(), filter.second);
            pos = message.find(token);
        }
    }
    return violation;
}


#endif