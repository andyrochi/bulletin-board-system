#ifndef BASE64_H
#define BASE64_H

char encoding[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "+/";
string encodingCpp(encoding);

string base64Encode(string rawString) {
    string result;
    int i;
    for (i = 0 ; i < rawString.size() ; i++) {
        int cur = rawString[i];
        if(i%3==0) {
            int index = (cur >> 2);
            result.push_back(encoding[index]);
        }
        else if(i%3==1) {
            int last = rawString[i-1];
            int index = ((last & 0x03) << 4) | (cur >> 4);
            result.push_back(encoding[index]);
        }
        else {
            int last = rawString[i-1];
            int index = ((last & 0x0F) << 2) | (cur >> 6);
            result.push_back(encoding[index]);
            index = cur & 0x3F;
            result.push_back(encoding[index]);
        }
    }
    if(rawString.size()%3==1){
        int last = rawString[i-1];
        int index = (last & 0x03) << 4;
        result.push_back(encoding[index]);
        result.push_back('=');
        result.push_back('=');
    }
    if(rawString.size()%3==2){
        int last = rawString[i-1];
        int index = (last & 0x0F) << 2;
        result.push_back(encoding[index]);
        result.push_back('=');
    }

    return result;
}

string base64Decode(string encodedString){
    string result;
    for(int i = 4 ; i <= encodedString.size() ; i+=4){
        int first = encodingCpp.find(encodedString[i-4], 0);
        int second = encodingCpp.find(encodedString[i-3], 0);
        int third = encodingCpp.find(encodedString[i-2], 0);
        int fourth = encodingCpp.find(encodedString[i-1], 0);

        char letter1 = (first << 2) | (second >> 4);
        result.push_back(letter1);
        if(third == string::npos) break;
        char letter2 = ((second & 0xF) << 4) | (third >> 2);
        result.push_back(letter2);
        if(fourth == string::npos) break;
        
        char letter3 = ((third & 0x3) << 6) | fourth;
        result.push_back(letter3);
    }
    return result;
}

#endif