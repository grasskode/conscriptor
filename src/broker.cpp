/*
 * Simple broker for a cloud. Just publishes its health to the cloud monitor for the time being.
 * Add functionality :
 *      # Register worker nodes in the cloud
 *      # Peer with other brokers in the same cloud
 *      # Route jobs to workers and route back responses
 */

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
        zmq::socket_t heartbeat (context, ZMQ_DEALER);
        heartbeat.connect("ipc://cloudmonitor.ipc");
	
	while(1){
		zmq::message_t health(100);
		snprintf((char*) health.data(), 100, "__CLOUD %s %s", name.c_str(), address.c_str());
		heartbeat.send(health);

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
