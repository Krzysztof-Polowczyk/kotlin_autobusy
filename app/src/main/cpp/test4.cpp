//
// Created by KrzysztofPolowczyk on 29.03.2026.
//
#include "test4.h"
#include "stdlib.h"
#include "stdio.h"

#include <iostream>
#include <vector>
#include "string.h"
#include<unistd.h>

#include <android/log.h>



std::vector<std::string> global_list = {};

std::vector<std::string> capitalize(const char *in) {
    std::vector<std::string> out = {};
    if (!*in){
        return out;
    }
    int num = atoi(in);

    for (int i = 0; i < num; i++){
        out.push_back(std::to_string(i));
    }

    return out;
}