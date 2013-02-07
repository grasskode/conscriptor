/*
 * broker.cpp
 *
 * Simple broker for a cloud.
 * Has the following functionalities :
 *    # Transmit the cloud heath to the monitor
 *    # Register worker nodes in the cloud
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
#include <zhelpers.hpp>

using namespace std;

static string name;
static string be_address;
static string fe_address;
static deque<string> nodes;

static void* node_listen(void* params) {
  zmq::context_t context(1);
  zmq::socket_t broker (context, ZMQ_DEALER);
  broker.bind(be_address.c_str());

  while(1) {
    zmq::message_t node_address;
    string address;
    string status;
    
    broker.recv(&node_address);
    istringstream iss(static_cast<char*>(node_address.data()));
    iss >> address >> status;

    cout<< name <<" : received address : "<<address<<endl;
    
    if(find(nodes.begin(), nodes.end(), address) == nodes.end()){
      cout<<name<<" : adding "<<address<<endl;
      nodes.push_back(address);
      cout<<name<<" : health = "<<nodes.size()<<endl;
    }
  }

}

//static void* node_speak(void* params) {
//  zmq::context_t context(1);
//  zmq::socket_t worker(context, ZMQ_REQ);
//  worker.connect("ipc://node.ipc");
//
//  while(1) {
//    cout<< "req : sending hello"<<endl;
//    s_send(worker, "hello");
//
//    string resp = s_recv(worker);
//    cout<<"response from worker : "<<resp<<endl;
//
//    sleep(5);
//  }
//}

static void* cloud_speak(void* params){
  zmq::context_t context (1);
  zmq::socket_t heartbeat (context, ZMQ_DEALER);
  heartbeat.connect("ipc://cloudmonitor.ipc");
	
	while(1){
		zmq::message_t health(100);
		snprintf((char*) health.data(), 100, "__CLOUD %s %s %lu", name.c_str(), fe_address.c_str(), nodes.size());
		heartbeat.send(health);

		sleep(5);
	}
}
 
static void route_requests() {
  zmq::context_t context(1);
  zmq::socket_t client(context, ZMQ_ROUTER);
  client.bind(fe_address.c_str());

  while(1) {
    string address = s_recv(client);
    s_recv(client);
    string request = s_recv(client);

    if(strcmp(request.c_str(), "ping") == 0) {
      s_sendmore(client, address);
      s_sendmore(client, "");
      s_send(client, "pong");
    }
    else{
      string next = nodes.front();
      nodes.pop_front();
      cout<<"worker address : "<<next<<endl;

      zmq::socket_t worker(context, ZMQ_DEALER);
      worker.connect(next.c_str());      
      s_sendmore(worker, address);
      s_sendmore(worker, "");
      s_send(worker, request);

      zmq::message_t response;
      worker.recv(&response);
      client.send(response);
    }
  }
}

int main(int argc, char* argv[]){
	if(argc < 2) {
		cout<<"Usage : broker <name>"<<endl;
		return -1;
	}
	
	name = argv[1];
	be_address = "ipc://"+name+"-be.ipc";
  fe_address = "ipc://"+name+"-fe.ipc";

  pthread_t speakd, listend;
  pthread_create(&speakd, NULL, cloud_speak, NULL);
  pthread_create(&listend, NULL, node_listen, NULL);

  route_requests();

  pthread_join(speakd, NULL);
  pthread_join(listend, NULL);

  return 0;
}
