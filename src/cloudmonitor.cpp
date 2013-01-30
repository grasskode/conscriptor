/*
 * Does not do much at the moment. Aggregates data from the clouds and publishes to the clients.
 * Need to add more functionality :
 *      # Store the health of the cloud and support query for the same.
 */

#include <zmq.hpp>
#include <iostream>

using namespace std;

static void* monitor(void* param) {
        zmq::context_t context(1);
        zmq::socket_t aggregator(context, ZMQ_DEALER);
        aggregator.bind("ipc://cloudmonitor.ipc");

        zmq::socket_t publisher(context, ZMQ_PUB);
        publisher.bind("ipc://cloudlist.ipc");

        while(1) {
                // Simply reads from the clouds and publishes to clients
                zmq::message_t cloudinfo;
                aggregator.recv(&cloudinfo);
                publisher.send(cloudinfo);
        }

}

int main(int argc, char* argv[]) {
        monitor(NULL);
        return 0;
}
