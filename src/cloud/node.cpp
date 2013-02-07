/*
 * Node that is the end point of all requests in the system.
 *    # Speaks to the broker about its status
 *    # Takes requests from broker and returns a result
 *    # TODO make the node return an immediate job id and return a result asynchronously to the client later
 */
#include <zmq.hpp>
#include <string>
#include <zhelpers.hpp>

using namespace std;

static string cloud;
static string name;
static string my_address;
static string broker_address;

static void* broker_speak(void* params) {
  zmq::context_t context (1);
  zmq::socket_t heartbeat (context, ZMQ_DEALER);
  heartbeat.connect(broker_address.c_str());
  
  while(1){
    zmq::message_t health(100);
    snprintf((char*) health.data(), 100, "%s %s", my_address.c_str(), "READY");
    heartbeat.send(health);

    sleep(5);
  }
}

static void broker_listen() {
  zmq::context_t context (1);
  zmq::socket_t broker(context, ZMQ_REP);
  broker.bind(my_address.c_str());

  while(1) {
    //string address = s_recv(worker);
    // discard empty delimiter
    //s_recv(worker);
    string req = s_recv(broker);
    cout << "router : request recieved : " << req << endl;

    //s_sendmore(broker, address);
    //s_sendmore(broker, "");
    s_send(broker, req);
  }
}

int main(int argc, char* argv[]) {
  if(argc < 3) {
    cout<<"Usage : node <node-name> <cloud-name>"<<endl;
    exit(1);
  }

  name = argv[1];
  cloud = argv[2];
  my_address = "ipc://"+name+".ipc";
  broker_address = "ipc://"+cloud+"-be.ipc";

  pthread_t speakd, listend;
  pthread_create(&speakd, NULL, broker_speak, NULL);

  broker_listen();

  exit(0);
}
