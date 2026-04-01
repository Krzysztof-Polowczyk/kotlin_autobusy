#include <jni.h>
#include <string>
#include "test4.hpp"
#include <jni.h>
#include <pthread.h>
#include <unistd.h> // for sleep
#include <string>
#include <cstring>
#include <stdatomic.h>
#include <iostream>
#include <vector>
#include <numeric>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>       // close()
#include <arpa/inet.h>    // sockaddr_in, inet_addr

struct ThreadData {
    JavaVM* jvm;
    jobject callbackGlobalRef;
};

typedef struct myTree{
    char is_word = 0;
    myTree* childs[256] = {};
} myTree;


myTree globalTree = {0,{}};


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int getSocket(){

    int sock;
    struct sockaddr_in server_addr;

    // 1. Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        __android_log_print(ANDROID_LOG_DEBUG, "MyNativeCode", "socket failed");

        exit(EXIT_FAILURE);
    }

    // 2. Server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(54000);
    server_addr.sin_addr.s_addr = inet_addr("192.168.1.228"); // localhost

    // 3. Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        __android_log_print(ANDROID_LOG_DEBUG, "MyNativeCode", "connect failed");

        close(sock);
        exit(EXIT_FAILURE);
    }

    return sock;
    // 6. Close socket
};

void add_to_tree(myTree* tree/*not NULL*/,const char* a/*delimited*/){

    char letter = a[0];

    if (letter == 0){
        tree->is_word = 1;
        return;
    }

    myTree** next = tree->childs + letter;


    if (*next == 0){
        *next = (myTree*)calloc(1,sizeof(myTree));
    }

    add_to_tree(*next, a + 1);
}

std::vector<std::string> getWords(myTree* tree, std::string prefix){
    if (tree == 0){
        return {};
    }
    std::vector<std::string> rest = {};
    for (int i = 0; i < 256; i++){

        std::string pass(prefix);
        pass.push_back(i);
        std::vector<std::string> toAdd = getWords(tree->childs[i], pass);
        rest.insert(rest.end(),toAdd.begin(),toAdd.end());
    }
    if (tree->is_word){
        rest.push_back(prefix);
    }
    return rest;
}

// Function executed by pthread
void* threadFunc(void* arg) {
    ThreadData* data = static_cast<ThreadData*>(arg);

    JNIEnv* env;
    jmethodID onResultMethod;
    jclass callbackClass;

    data->jvm->AttachCurrentThread((JNIEnv **)&env, nullptr);
    callbackClass = env->GetObjectClass(data->callbackGlobalRef);
    onResultMethod = env->GetMethodID(callbackClass, "onResult", "(Ljava/lang/String;)V");
    jstring result = env->NewStringUTF("hej");

    int i = 0;
    int sock = getSocket();
    char buffer[1024] = {};
    int n;


    while (1){
        //__android_log_print(ANDROID_LOG_DEBUG, "MyNativeCode", "wile done");

        sleep(2);

        n = read(sock, buffer, sizeof(buffer)-1);
        buffer[sizeof(buffer)-1] = '\0';
        if (n > 0) {
            add_to_tree(&globalTree, buffer);

        }

        pthread_mutex_lock(&lock);


        pthread_mutex_unlock(&lock);

        i++;

        env->CallVoidMethod(data->callbackGlobalRef, onResultMethod, result);
    };

    // Clean up
    env->DeleteLocalRef(result);
    env->DeleteLocalRef(callbackClass);

    data->jvm->DetachCurrentThread();
    env->DeleteGlobalRef(data->callbackGlobalRef);
    close(sock);

    delete data;

    return nullptr;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_test4_MainActivity_startThread(JNIEnv *env, jobject thiz, jobject callback) {

    JavaVM* jvm;
    env->GetJavaVM(&jvm);

    // Make global reference to callback
    jobject globalCallback = env->NewGlobalRef(callback);

    // Allocate data for pthread
    ThreadData* data = new ThreadData();
    data->jvm = jvm;
    data->callbackGlobalRef = globalCallback;

    // Create pthread
    pthread_t thread;
    pthread_create(&thread, nullptr, threadFunc, data);
    pthread_detach(thread); // detach to avoid needing pthread_join
}
extern "C" JNIEXPORT jobjectArray   JNICALL
Java_com_example_test4_MainActivity_capitalize(
        JNIEnv* env,
        jobject,
        jstring in
) {
    const char* inputChars = env->GetStringUTFChars(in, nullptr);

    std::vector<std::string> out;
    pthread_mutex_lock(&lock);
    myTree* currentRoot = &globalTree;
    for(int i = 0; inputChars[i]; i++){
        if (currentRoot->childs[inputChars[i]] == 0) goto label;
        currentRoot = currentRoot->childs[inputChars[i]];
    }
    out = getWords(currentRoot, inputChars);

    label:
    pthread_mutex_unlock(&lock);

    jobjectArray jarray = env->NewObjectArray(
            out.size(),
            env->FindClass("java/lang/String"),
            nullptr
    );

    env->ReleaseStringUTFChars(in,inputChars);

    for (size_t i = 0; i < out.size(); ++i) {
        env->SetObjectArrayElement(jarray, i, env->NewStringUTF(out[i].c_str()));
    }



    return jarray;
}


