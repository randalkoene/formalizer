// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <new>
#include <stdlib.h>

// core
//#include "error.hpp"
#include "cgihandler.hpp"

namespace fz {

// Adapted from https://www.guyrutenberg.com/2007/09/07/introduction-to-c-cgi-processing-forms/
std::string URL_Decode(const std::string str) {
    std::string temp;
    temp.reserve(str.size());
    char tmp[5] = {0, 0, 0, 0, 0};
    char tmpchar;

    int size = str.size();
    for (int i = 0; i < size; i++) {
        if (str[i] == '%') {
            if (i + 2 < size) {
                tmp[2] = str[i + 1];
                tmp[3] = str[i + 2];
                tmp[4] = '\0';
                tmpchar = (char)strtol(tmp, NULL, 0);
                temp += tmpchar;
                i += 2;
                continue;
            } else {
                break;
            }
        } else if (str[i] == '+') {
            temp += ' ';
        } else {
            temp += str[i];
        }
    }
    return temp;
}

void CGI_Token_Values::decode_GET_query_string() {
    std::string tmpkey, tmpvalue;
    std::string *tmpstr = &tmpkey;

    char * raw_get = getenv("QUERY_STRING");
    if (raw_get == NULL) {
        GET.clear();
        return;
    }
    has_query_string = true;

    while (*raw_get != '\0') {
        if (*raw_get == '&') {
            if (tmpkey != "") {
                GET[URL_Decode(tmpkey)] = URL_Decode(tmpvalue);
            }
            tmpkey.clear();
            tmpvalue.clear();
            tmpstr = &tmpkey;
        } else if (*raw_get == '=') {
            tmpstr = &tmpvalue;
        } else {
            (*tmpstr) += (*raw_get);
        }
        raw_get++;
    }
    //enter the last pair to the map
    if (tmpkey != "") {
        GET[URL_Decode(tmpkey)] = URL_Decode(tmpvalue);
        tmpkey.clear();
        tmpvalue.clear();
    }
}

void CGI_Token_Values::decode_POST_data() {
    std::string tmpkey, tmpvalue;
    std::string *tmpstr = &tmpkey;
    
    char *strlength = getenv("CONTENT_LENGTH");
    if (strlength == NULL) {
        POST.clear();
        return;
    }
    has_post_length = true;
    int content_length = atoi(strlength);
    if (content_length == 0) {
        POST.clear();
        return;
    }

    char *buffer = NULL;
    try {
        buffer = new char[content_length * sizeof(char) + 1];
    } catch (const std::bad_alloc & xa) {
        POST.clear();
        return;
    }
    if (fread(buffer, sizeof(char), content_length, stdin) != (unsigned int)content_length) {
        POST.clear();
        return;
    }
    buffer[content_length] = '\0';

    char * ibuffer = buffer;
    while (*ibuffer != '\0') {
        if (*ibuffer == '&') {
            if (tmpkey != "") {
                POST[URL_Decode(tmpkey)] = URL_Decode(tmpvalue);
            }
            tmpkey.clear();
            tmpvalue.clear();
            tmpstr = &tmpkey;
        } else if (*ibuffer == '=') {
            tmpstr = &tmpvalue;
        } else {
            (*tmpstr) += (*ibuffer);
        }
        ibuffer++;
    }
    //enter the last pair to the map
    if (tmpkey != "") {
        POST[URL_Decode(tmpkey)] = URL_Decode(tmpvalue);
        tmpkey.clear();
        tmpvalue.clear();
    }
}

} // namespace fz
