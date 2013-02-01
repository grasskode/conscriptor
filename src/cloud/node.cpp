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

static void* broker_listen(void* params) {
  zmq::context_t context (1);
  zmq:: socket_t worker(context, ZMQ_ROUTER);
  worker.bind(my_address.c_str());

  while(1) {
    s_dump(worker);
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
  broker_address = "ipc://"+cloud+".ipc";

  //pthread_t speakd, listend;
  //pthread_create(&speakd, NULL, broker_speak, NULL);
  //pthread_create(&listend, NULL, broker_listen, NULL);

  broker_listen(NULL);

  //pthread_join(speakd, NULL);
  //pthread_join(listend, NULL);

  exit(0);
}
