#include <jni.h>
#include <string>
#include "test4.h"
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
#include <signal.h>
#include <unistd.h>
#include <format>

thread_local int halt = 0;
#include <sched.h>

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

int getSocket(int port, char const *ip){

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
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip); // localhost

    // 3. Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        __android_log_print(ANDROID_LOG_DEBUG, "MyNativeCode", "connect failed");

        close(sock);
        exit(EXIT_FAILURE);
    }

    return sock;
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

    pthread_mutex_lock(&lock);
    add_to_tree(&globalTree, "testtt");
    pthread_mutex_unlock(&lock);

    int i = 0;
    int sock = getSocket(54910, "192.168.1.228");
    char buffer[64] = {};
    int n;

    //temp deboug remove
    // end


    while (1){

        n = read(sock, buffer, sizeof(buffer));
        buffer[sizeof(buffer)-1] = '\0';

        if (n > 0) {
            pthread_mutex_lock(&lock);
            add_to_tree(&globalTree, buffer);
            pthread_mutex_unlock(&lock);
        } else {
            close(sock);
            return NULL;
        }


        __android_log_print(ANDROID_LOG_DEBUG, "MyNativeCode", "wile done %s", buffer);

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


    pthread_t thread;
    pthread_create(&thread, nullptr, threadFunc, data);
    pthread_detach(thread);
}


typedef struct __attribute__((packed)) train_arives {
    char time[10];
    int line;
    int vech;
    char delimiter;
} train_arives;

pthread_mutex_t fetch_mutex, safty_mutex;
std::vector<std::string> fetched_out;

pthread_t fetching_thread;

void handler(int sig) {
    __android_log_print(ANDROID_LOG_DEBUG, "MyNativeCode", "terminating");
    halt = 1;
}

volatile ThreadData *on_fetched_call_back;

void* SIG_REC(void *args){
    std::vector<std::string> out = {};
    const char* request = (const char*)args;

    signal(22, handler);
    int sock = getSocket(54911, "192.168.1.228");

    write(sock, request, strlen(request));
    train_arives temp;
    while(!halt) {
        sleep(1);
        if (read(sock,&temp, sizeof(train_arives)) == 0) break;



        out.push_back(std::string(temp.time) + " " + std::to_string(temp.line) + " " + std::to_string(temp.vech));
        __android_log_print(ANDROID_LOG_DEBUG, "MyNativeCode", "read %s %d", temp.time, sizeof(train_arives));
    }

    if (!halt){
        pthread_mutex_lock(&safty_mutex);
        fetched_out = out;

        JNIEnv* env;

        jmethodID onResultMethod;
        jclass callbackClass;

        //__android_log_print(ANDROID_LOG_DEBUG, "MyNativeCode", "hejjjjjjjj");
        on_fetched_call_back->jvm->AttachCurrentThread((JNIEnv **)&env, nullptr);
        callbackClass = env->GetObjectClass(on_fetched_call_back->callbackGlobalRef);
        onResultMethod = env->GetMethodID(callbackClass, "onResult", "(Ljava/lang/String;)V");
        jstring result = env->NewStringUTF("hej");

        env->CallVoidMethod(on_fetched_call_back->callbackGlobalRef, onResultMethod, result);
    }

    pthread_mutex_unlock(&fetch_mutex);
    close(sock);

    while(!halt){
        sched_yield();
    };


    return nullptr;
}



extern "C" JNIEXPORT void JNICALL
Java_com_example_test4_MainActivity_FetchDataInit(JNIEnv *env, jobject thiz, jobject callback) {
    JavaVM* jvm;
    env->GetJavaVM(&jvm);

    // Make global reference to callback
    jobject globalCallback = env->NewGlobalRef(callback);

    // Allocate data for pthread
    ThreadData* data = new ThreadData();
    data->jvm = jvm;
    data->callbackGlobalRef = globalCallback;


    on_fetched_call_back = data;

    pthread_mutex_init(&fetch_mutex, NULL);
    pthread_mutex_init(&safty_mutex, NULL);
}



extern "C" JNIEXPORT void JNICALL
Java_com_example_test4_MainActivity_FetchData(JNIEnv *env, jobject thiz, jstring in) {
    const char* inputChars = env->GetStringUTFChars(in, nullptr);

    if (pthread_kill(fetching_thread, 22) != 0) {};

    pthread_mutex_lock(&fetch_mutex);

    pthread_create(&fetching_thread, nullptr, SIG_REC, (void*)inputChars);
    pthread_detach(fetching_thread);


    __android_log_print(ANDROID_LOG_DEBUG, "MyNativeCode", "empty");

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



extern "C" JNIEXPORT jobjectArray   JNICALL
Java_com_example_test4_MainActivity_getFetchedData(
        JNIEnv* env,
        jobject
) {
    __android_log_print(ANDROID_LOG_DEBUG, "MyNativeCode", "alex");


    jobjectArray jarray = env->NewObjectArray(
            fetched_out.size(),
            env->FindClass("java/lang/String"),
            nullptr
    );

    for (size_t i = 0; i < fetched_out.size(); ++i) {
        env->SetObjectArrayElement(jarray, i, env->NewStringUTF(fetched_out[i].c_str()));
    }
    pthread_mutex_unlock(&safty_mutex);
    return jarray;
}




