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
#include <vector>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>

using namespace std;

int listening_socket;
int client_socket;
int flag_treatment = 0;
int flag_waiting = 0;
int flag_reception = 0;
pthread_mutex_t mutex;
pthread_t treatment_thread;
pthread_t reception_thread;
vector <string> msglist;

void signal_handler(int signo)
{
     int *exitcode1;
     int *exitcode2;

     if (pthread_join(reception_thread, (void**)&exitcode1)){
        cout << "pthread_join" << strerror(errno) << endl;
     } else {
        cout << "Exitcode: " << exitcode1 << endl;
     }

     if (pthread_join(treatment_thread, (void**)&exitcode2)){
        cout << "pthread_join" << strerror(errno) << endl;
     } else {
        cout << "Exitcode: " << exitcode2 << endl;
     }

     if (shutdown(listening_socket, 2) == -1){
         cout << "shutdown: " << strerror(errno) << endl;
     }

     if (close(client_socket) == -1){
        cout << "close: " << strerror(errno) << endl;
     }

    if (close(listening_socket) == -1){
        cout << "close: " << strerror(errno) << endl;
    }

    if (pthread_mutex_destroy(&mutex)){
        cout << "pthread_mutex_destroy: " << strerror(errno) << endl;
    }
}

void *reception_function (void *arg)
{
    int *args = (int*)arg;
    cout << "Reception function on" << endl;
    while(*args == 0)
    {
        char buffer[50];

        int reccount = recv(client_socket, (void*)&buffer, sizeof(buffer), 0);
        if (reccount == -1){
            cout << "recv " << strerror(errno) << endl;
            sleep(1);
        } else if (reccount == 0){
            if (shutdown(client_socket, SHUT_RDWR) == -1){
                cout << "close: " << strerror(errno) << endl;
            }
        } else{
            if (pthread_mutex_lock(&mutex)){
                cout << "pthread_mutex_lock: " << strerror(errno) << endl;
            }

            msglist.push_back(string(buffer));

            if (pthread_mutex_unlock(&mutex)){
                cout << "pthread_mutex_unlock: " << strerror(errno) << endl;
            }
        }
        cout << "Client request: " << buffer << endl;
        sleep(1);
    }
    cout << "Reception function is off" << endl;
    pthread_exit((void*)1);
}


void *treatment_function(void *arg)
{
    int *args = (int*)arg;
    cout << "Treatment function on" << endl;
    while(*args == 0)
    {
        char buffer[50];
        if (pthread_mutex_lock(&mutex)){
            cout << "pthread_mutex_lock: " << strerror(errno) << endl;
        }
        if (!msglist.empty())
        {
            string S = msglist.back();
            msglist.pop_back();

            if (pthread_mutex_unlock(&mutex)){
                cout << "pthread_mutex_unlock: " << strerror(errno) << endl;
            }
            int priority = getpriority(PRIO_PROCESS, 0);
            if (priority == -1) {
               cout << "getpriority: " << strerror(errno) << endl;
            } else {
               cout << "Thread priority: " << priority << endl;
            }

            sprintf(buffer, "Reached priority is: %d", priority);

            if (send(client_socket, buffer, strlen(buffer), 0) == -1){
                cout << "send: " << strerror(errno) << endl;
            }
        } else{
            if (pthread_mutex_unlock(&mutex)){
                cout << "pthread_mutex_unlock: " << strerror(errno) << endl;
            }
        }
        cout << buffer << endl;
        sleep(1);
    }
    cout << "Treatment function off" << endl;
    pthread_exit((void*)1);
}


void *waiting_function(void *arg)
{
    int *args = (int*)arg;
    cout << "Waiting function is on" << endl;
    while(*args == 0)
    {
        struct sockaddr addr;
        socklen_t addrlen;
        client_socket = accept(listening_socket, &addr, &addrlen);
        if (client_socket == -1){
            cout << "accept: " << strerror(errno) << endl;
            sleep(1);
        } else{
            if (fcntl(client_socket, F_SETFL, O_NONBLOCK) == -1){
                cout << "fcntl: " << strerror(errno) << endl;
            }

                cout << "Client server addr: " << addr.sa_data << endl;

                if (pthread_create(&reception_thread, NULL, reception_function, &flag_reception)){
                    cout << "pthread_create: " << strerror(errno) << endl;
                    pthread_exit((void*)11);
                }

                if (pthread_create(&treatment_thread, NULL, treatment_function, &flag_treatment)){
                    cout << "pthread_create: " << strerror(errno) << endl;
                    pthread_exit((void*)12);
                }
                cout << "Waiting function off" << endl;
                pthread_exit((void*)1);
        }
    }
    pthread_exit((void*)49);
}



int main(){
   pthread_t waiting_thread;
   int optval = 1;
   int *exitcode1;
   int *exitcode2;
   int *exitcode3;

   struct sockaddr_un ServerAddres;

   cout << "Programm 1 is running" << endl;

   //Socket creation

   listening_socket = socket (AF_UNIX, SOCK_STREAM, 0);
   if (listening_socket == -1){
      cout << "socket: " << strerror(errno) << endl;
   } else {
      cout << "Socket creation done: " << listening_socket << endl;
   }

   if (fcntl(listening_socket, F_SETFL, O_NONBLOCK) == -1){
      cout << "fcntl: " << strerror(errno) << endl;
   }

   //Server attributes

   ServerAddres.sun_family = AF_UNIX;
   strcpy(ServerAddres.sun_path, "server.soc");

   //port binding

   if (bind(listening_socket, (const sockaddr*)&ServerAddres, socklen_t(sizeof(ServerAddres))) == -1){
      cout << "bind: " << strerror(errno) << endl;
   } else {
      cout << "Port is binded" << endl;
   }

   if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1){
      cout << "setsockopt: " << strerror(errno) << endl;
   }

   if (listen(listening_socket, 10) == -1){
      cout << "listen: " << strerror(errno) << endl;
   }

   //mutex init

   if (pthread_mutex_init(&mutex, NULL)){
      cout << "pthread_mutex_init: " << strerror(errno) << endl;
   } else {
      cout << "mutex init" << endl;
   }

   //thread creation

   if (pthread_create(&waiting_thread, NULL, waiting_function, &flag_waiting)){
        cout << "listen: " << strerror(errno) << endl;
        return 1;
    }

    getchar();
    cout << "Enter was pressed" << endl;
    flag_treatment = 1;
    flag_waiting = 1;
    flag_reception = 1;

    //Exitcodes

    if (pthread_join(reception_thread, (void**)&exitcode1)){
        cout << "pthread_join: " << strerror(errno) << endl;
    }
    else{
    cout<<"Exitcode 1: "<< exitcode1 <<endl;
    }
    if (pthread_join(treatment_thread, (void**)&exitcode2)){
        cout << "pthread_join: " << strerror(errno) << endl;
    }
    else{
    cout<<"Exitcode 2: "<< exitcode2 <<endl;
    }
    if (pthread_join(waiting_thread, (void**)&exitcode3)){
        cout << "pthread_join: " << strerror(errno) << endl;
    }
    else{
    cout<<"Exitcode 3: "<< exitcode3 <<endl;
    }

    //shutdown

    if (shutdown(listening_socket, 2) == -1){
       cout << "pthread_join: " << strerror(errno) << endl;
    } else {
       cout << "listening_socket was shut" << endl;
    }

    if (close(client_socket) == -1){
       cout << "close: "<< strerror(errno) << endl;
    }

    if (close(listening_socket) == -1){
       cout << "close: "<< strerror(errno) << endl;
    }

    //mutex destroy

    if (pthread_mutex_destroy(&mutex)){
       cout << "close: "<< strerror(errno) << endl;
    }

    return 0;
}
