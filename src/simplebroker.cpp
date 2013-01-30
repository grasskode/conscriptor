#include <zmq.hpp>
#include <unistd.h>
#include <string>
#include <stdio.h>
#include <iostream>

using namespace std;

static string name;
static string address;

static void speak(){
        zmq::context_t context (1);
        zmq::socket_t publisher (context, ZMQ_PUB);
        publisher.bind("ipc://cloudlist.ipc");
	
	while(1){
		zmq::message_t health(100);
		snprintf((char*) health.data(), 100, "__CLOUD %s %s", name.c_str(), address.c_str());
		publisher.send(health);

		sleep(5);
	}
}

int main(int argc, char* argv[]){
	if(argc < 2) {
		cout<<"Usage : simplebroker <name>"<<endl;
		return -1;
	}
	
	name = argv[1];
	address = "ipc://"+string(argv[1])+".ipc";
        speak();
        return 0;
}
