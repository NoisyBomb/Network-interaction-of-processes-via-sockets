#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <grp.h>


using namespace std;

int flag_connection = 0;
int flag_transfer = 0;
int flag_answer_reception = 0;
int socket_in;
pthread_t transfer_thread;
pthread_t answer_reception_thread;


void signal_handler(int signo)
{
    int *exitcode1;
    int *exitcode2;

    if (pthread_join(transfer_thread, (void**)&exitcode1)){
        cout << "pthread_join: " << strerror(errno) << endl;
    }else {
        cout << "Exitcode1: " << exitcode1 << endl;
    }

    if (pthread_join(answer_reception_thread, (void**)&exitcode2)){
        cout << "pthread_join: " << strerror(errno) << endl;
    }else {
        cout << "Exitcode2: " << exitcode1 << endl;
    }

    if (shutdown(socket_in, 2) == -1){
        cout << "shutdown: " << strerror(errno) << endl;
    }
    if (close(socket_in) == -1){
        cout << "close: " << strerror(errno) << endl;
    }
}

void *transfer_function(void *arg)
{
    int *args = (int*)arg;
    cout << "transfer_function is on" << endl;
    size_t itteration;
    while(*args == 0)
    {
        char buffer[200];
        itteration++;
        struct sockaddr_un clAddr;
        clAddr.sun_family = AF_UNIX;
        strcpy(clAddr.sun_path, "client.soc");
        socklen_t namelen;
        if (getsockname(socket_in, (struct sockaddr*)&clAddr, &namelen) == -1){
            cout << "getsockname: " << strerror(errno) << endl;
        }
        sprintf(buffer, "Request number from client is %zu. Client socket address: %s\n", itteration, clAddr.sun_path);
        if (send(socket_in, buffer, strlen(buffer), 0) == -1){
            cout << "send: " << strerror(errno) << endl;
        }
        cout << buffer << endl;
        sleep(1);

    }
    cout << "transfer_function is off" << endl;
    pthread_exit((void*)1);
}

void *answer_reception_function(void *arg)
{
    int *args = (int*)arg;
    cout << "answer_reception_function is on" << endl;
    size_t itteration = 0;
    while(*args == 0)
    {
        char buffer[200];
        ssize_t reccount = recv(socket_in, buffer, sizeof(buffer), 0);
        if (reccount == -1)
        {
            cout << "recv: " << strerror(errno) << endl;
            sleep(1);
        } else if (reccount == 0){
            if (shutdown(socket_in, 2) == -1){
                cout << "getsockname: " << strerror(errno) << endl;
                pthread_exit((void*)3);
            }
            sleep(1);
        }else {
            itteration++;
            cout << "Server answer is: " << itteration << " " << buffer;
        }
    }
    cout << "answer_reception_function is off" << endl;
    pthread_exit((void*)1);
}

void *connection_function(void *arg)
{
    int *args = (int*)arg;
    cout << "connection_function is on" << endl;
    while(*args == 0)
    {
        struct sockaddr_un serverAddr;
        serverAddr.sun_family = AF_UNIX;
        char name[100] = "server.soc";

        strcpy(serverAddr.sun_path, name);
        if (connect(socket_in, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1){
            cout << "connect: " << strerror(errno) << endl;
            sleep(1);
        }else {
            cout << "Function connected" << endl;
            if (pthread_create(&answer_reception_thread, NULL, answer_reception_function, &flag_answer_reception)){
                cout << "pthread_create: " << strerror(errno) << endl;
                pthread_exit((void*)19);
            }
            if (pthread_create(&transfer_thread, NULL, transfer_function, &flag_transfer)){
                cout << "pthread_create: " << strerror(errno) << endl;
                pthread_exit((void*)12);
            }
            cout << "connection fumction off" << endl;
            pthread_exit((void*)1);
        }
    }
    pthread_exit((void*)49);
}

int main()
{
    int *exitcode1;
    int *exitcode2;
    int *exitcode3;

    signal(SIGPIPE, signal_handler);
    struct sockaddr_un clientAddr;
    cout << "Prog 2 is running" << endl;
    pthread_t connection_thread;
    socket_in = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_in == -1){
        cout << "socket: " << strerror(errno) << endl;
    }
    if (fcntl(socket_in, F_SETFL, O_NONBLOCK) == -1){
        cout << "fcntl: " << strerror(errno) << endl;
    }
    clientAddr.sun_family = AF_UNIX;
    strcpy(clientAddr.sun_path, "client.soc");
    if (bind(socket_in, (const sockaddr*)&clientAddr, sizeof(clientAddr)) == -1){
        cout << "fcntl: " << strerror(errno) << endl;
    }
    if (pthread_create(&connection_thread, NULL, connection_function, &flag_connection)){
        cout << "pthread_create: " << strerror(errno) << endl;
    }
    cout << "Waiting for key press" << endl;
    getchar();
    flag_connection = 1;
    flag_transfer = 1;
    flag_answer_reception = 1;
    if (pthread_join(transfer_thread, (void**)&exitcode1)){
        cout << "pthread_join: " << strerror(errno) << endl;
    }else {
        cout << "Exitcode 1: " << exitcode1 << endl;
    }
    if (pthread_join(answer_reception_thread, (void**)&exitcode2)){
        cout << "pthread_join: " << strerror(errno) << endl;
    }else {
       cout << "Exitcode 2: " << exitcode2 << endl;
    }
    if (pthread_join(connection_thread, (void**)&exitcode3)){
        cout << "pthread_join: " << strerror(errno) << endl;
    }else {
        cout << "Exitcode 3: " << exitcode3 << endl;
    }
    if (shutdown(socket_in, 2) == -1){
        cout << "shutdown: " << strerror(errno) << endl;
    }
    if (close(socket_in) == -1){
        cout << "close: " << strerror(errno) << endl;
        return 10;
    }

    cout << "Prog 2 was illuminated" << endl;
    return 0;
}
