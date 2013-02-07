/*
 * client.cpp
 *
 * Simple client that does the following :
 *    # Subscribe to the cloud listing
 *    # list available clouds
 *    # ping a given address
 */

#include "cloudlistenerd.cpp"
#include <dirent.h>
#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <pthread.h>
#include <fstream>
#include <zhelpers.hpp>

using namespace std;

static const string CLOUD_BASE = "/home/karan/workspace/conscriptor/cloud";
static const int CLEANUP_INTERVAL = 10;

void list();
void ping(string address);
string submit(string address, string workload);

int main (int argc, char* argv[]) {
  CloudListener listener(CLOUD_BASE, CLEANUP_INTERVAL);
  listener.activate();

  bool cont = true;
  string input;
  while(cont) {
    cout << ">> ";
    cin >> input;

    if(strcmp(input.c_str(), "help") == 0) {
      cout<<"Supported commands are :\n\tlist\n\texit"<<endl;
    }
    else if(strcmp(input.c_str(), "list") == 0) {
      list();
    }
    else if(strcmp(input.c_str(), "ping") == 0) {
      string address;
      cin>>address;
      ping(address);
    }
    else if(strcmp(input.c_str(), "submit") == 0) {
      string address, workload;
      cin>>address>>workload;
      string result = submit(address, workload);
      cout<< "submission result : "<<result<<endl;
    }
    else if(strcmp(input.c_str(), "exit") == 0) {
      listener.terminate();
      cont = false;
    }
    else {
      cout<< "Invalid input. Use help to get the supported commands."<<endl;
    }
  }
}

/*
 * Read from the cloud listing and publish the list of available clouds and their details
 */
void list() {
  DIR* cloud_base;
  if((cloud_base = opendir(CLOUD_BASE.c_str())) != NULL) {
    struct dirent *c_ent;
    struct stat st;
    while((c_ent = readdir(cloud_base)) != NULL) {
      if(strcmp(c_ent->d_name, ".") == 0 || strcmp(c_ent->d_name, "..") == 0) continue;
      cout<<"** "<<c_ent->d_name<<endl;
      string c_path = CLOUD_BASE+"/"+c_ent->d_name;
      stat(c_path.c_str(), & st);
      if(S_ISDIR(st.st_mode)) {
        DIR* cloud;
        if(( cloud = opendir(c_path.c_str())) != NULL) {
          struct dirent* n_ent;
          while(( n_ent = readdir(cloud)) != NULL) {
            if(strcmp(n_ent->d_name, ".") == 0 || strcmp(n_ent->d_name, "..") == 0) continue;
            string n_path = c_path+"/"+n_ent->d_name;
            stat(n_path.c_str(), &st);
            if(S_ISREG(st.st_mode)) {
              // open the file and print its contents
              cout<<"\t**"<<endl;
              ifstream fin(n_path.c_str(), ifstream::in);
              while(!fin.eof()) {
                string line;
                fin >> line;
                cout << "\t" << line <<endl;
              }
              fin.close();
            }
          }
        }
      }
    }
    closedir(cloud_base);
  } else {
    perror(("Could not access cloud information at "+CLOUD_BASE).c_str());
  }
}

/*
 * Ping the given address and display the response time
 * TODO extend to pinging of cloud names as well
 */
void ping(string address) {
  double time = s_clock();
  string response = submit(address, "ping");
  cout<<"ping response in "<<(s_clock() - time)<<"ms : "<<response<<endl;
}

/*
 * Utility method to submit a workload to an address and get the result
 */
string submit(string address, string workload) {
  zmq::context_t context(1);
  zmq::socket_t submission(context, ZMQ_REQ);
  submission.connect(address.c_str());
  s_send(submission, workload);
  return s_recv(submission);
}

