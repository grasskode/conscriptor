/*
 * Simple broker for a cloud.
 * Has the following functionalities :
 *    # Transmit the cloud heath to the monitor
 *    # TODO Register worker nodes in the cloud
 *    # TODO Ping registered nodes regularly to ensure availability
 *    # TODO Peer with other brokers in the same cloud
 *    # TODO Route jobs to workers and route back responses
 */

#include <zmq.hpp>
#include <unistd.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <deque>
#include <sstream>

using namespace std;

static string name;
static string address;
static deque<string> nodes;

static void* node_listen(void* params) {
  zmq::context_t context(1);
  zmq::socket_t broker (context, ZMQ_DEALER);
  broker.bind(address.c_str());

  while(1) {
    zmq::message_t node_address;
    char* address;
    
    broker.recv(&node_address);
    istringstream iss(static_cast<char*>(node_address.data()));
    iss >> address;
    
    if(find(nodes.begin(), nodes.end(), address) == nodes.end()){
      cout<<name<<" : adding "<<address<<endl;
      nodes.push_back(address);
    }
  }

}

static void* node_speak(void* params) {
  zmq::context_t context(1);
  zmq::socket_t worker(context, ZMQ_REQ);
  worker.connect("ipc://node.ipc");

  zmq::message_t msg(5);
  snprintf((char*) msg.data(), 5, "hello");

  worker.send(msg);
}

static void* cloud_speak(void* params){
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
		cout<<"Usage : broker <name>"<<endl;
		return -1;
	}
	
	name = argv[1];
	address = "ipc://"+string(argv[1])+".ipc";

  //pthread_t speakd, listend;
  //pthread_create(&speakd, NULL, cloud_speak, NULL);
  //pthread_create(&listend, NULL, node_listen, NULL);

  node_speak(NULL);

  //pthread_join(speakd, NULL);
  //pthread_join(listend, NULL);

  return 0;
}
