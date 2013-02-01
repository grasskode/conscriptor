/*
 * Simple listener daemon that tracks the cloud status. Collects the data published by the monitor and
 * creates a corresponding folder structure for the available clouds and their broker nodes. Keeps a
 * track of active and inactive clouds.
 * TODO error checks and logging
 */

#include <zmq.hpp>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <typeinfo>

using namespace std;

class CloudListener {
  private :
    const char* BASE_DIR;
    int CLEANUP_INTERVAL;
    pthread_t lthread, cthread;
    bool TERM;
    
    /*
     * Return the input in a fs supported format
     */
    string format(const string input) {
      string output = "";
      for(int i=0; i<input.size(); i++) {
        if((input[i] >= 'a' && input[i] <= 'z') || (input[i] >= 'A' && input[i] <= 'Z') || (input[i] >= '0' && input[i] <= '9')) {
          output += input[i];
        } else {
          output += "_";
        }
      }
      return output;
    }
    
    /*
     * Update the given cloud name for the given address in the file system
     */
    int update(const string name, const string address) {
      string filepath = string(BASE_DIR)+"/"+name+"/";
      struct stat st;
      if(stat(filepath.c_str(), &st) == 0) {
        if(st.st_mode & S_IFDIR != 0 && S_ISDIR(st.st_mode))
          cout<<filepath<<" is present as a directory."<<endl;
        else
          cerr<<filepath<<" is present but is not a directory!"<<endl;
      } else {
        int status = mkdir(filepath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      }

      filepath += format(address);
      cout<<"Creating file at "<<filepath<<endl;
      FILE* file = fopen(filepath.c_str(), "w");
      if(file == 0) {
        cerr<<"Could not create file!"<<endl;
        return -1;
      }        
      time_t t = time(0);
      ostringstream oss;
      oss << address << "\t" << t <<endl;
      fputs(oss.str().c_str(), file);
      fclose(file);

      return 0;
    }
    
    /*
     * Daemon that cleans up the clou info in the node if there is no activity for 10 sec
     * TODO make this a configurable number
     */
    static void* cleanup(void* params){
      CloudListener* listener = (CloudListener*) params;
      while(!listener->TERM) {
        DIR* dp;
        if((dp = opendir(listener->BASE_DIR)) == 0) {
          cerr << errno <<" : Unable to open "<<listener->BASE_DIR<<" for cleanups! Aborting."<<endl;
        }
        else{
          // browse all directories in BASE_DIR
          struct dirent *dirp;
          while((dirp = readdir(dp)) != 0) {
            if(strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0) continue;
            string path = string(listener->BASE_DIR)+"/"+dirp->d_name;
            struct stat st;
            stat(path.c_str(), &st);
            if(S_ISDIR(st.st_mode)){
              DIR* cloud;
              if((cloud = opendir(path.c_str())) == 0) {
                cerr<<errno<<" : Unable to access cloud data at "<<path<<endl;
                continue;
              }
              struct dirent * cloudp;
              while((cloudp = readdir(cloud)) != 0) {
                if(strcmp(cloudp->d_name, ".") == 0 || strcmp(cloudp->d_name, "..") == 0) continue;
                // check last modified date for all files 
                string nodepath = path + "/" + cloudp->d_name;
                stat(nodepath.c_str(), &st);
                time_t modified = st.st_mtime;
                time_t current = time(NULL);
                cout<<"node "<<dirp->d_name<<"/"<<cloudp->d_name<<" idle for "<<current-modified<<" seconds."<<endl;
                if(current - modified >= 10) {
                  if( remove(nodepath.c_str()) != 0 ){
                    perror(("Error deleting "+nodepath).c_str());
                  }
                  else{
                    cout<<"node "<<dirp->d_name<<"/"<<cloudp->d_name<<" removed after inactivity of "<<current-modified<<" seconds."<<endl;
                  }
                }
              }
              closedir(cloud);
            }
          }
          closedir(dp);

          sleep(listener->CLEANUP_INTERVAL);
        }
      }
    }

    /*
     * Daemon that listens to the cloud list and updates the file system accordingly
     */
    static void* listen(void* params){
      CloudListener* listener = (CloudListener*) params;
      zmq::context_t context (1);
      zmq::socket_t subscriber (context, ZMQ_SUB);
      subscriber.connect("ipc://cloudlist.ipc");

      const char* filter = "__CLOUD ";
      subscriber.setsockopt(ZMQ_SUBSCRIBE, filter, strlen(filter));

      while(!listener->TERM) {
              zmq::message_t cloudinfo;
              string filter_ignore;
              string name;
              string address;

              if(!subscriber.recv(&cloudinfo, ZMQ_NOBLOCK)){
                // sleep and continue loop if no messages are waiting
                // this enables us to terminate the listener threads when required
                sleep(1);
                continue;
              }
              istringstream iss(static_cast<char*>(cloudinfo.data()));
              iss >> filter_ignore >> name >> address;
              cout<<"cloudlistenerd : "<<name<<" "<<address<<endl;
              if(listener->update(name, address) < 0)
                cerr<< "Unable to update cloud info for "<<name<<" "<<address<<endl;
      }
      
    }

  public :
    /*
     * Constructor
     */
    CloudListener(string basedir, int cleanup_interval) {
      BASE_DIR = basedir.c_str();
      CLEANUP_INTERVAL = cleanup_interval;
      TERM = false;
      lthread = -1;
      cthread = -1;
    }
    
    /*
     * Activates the listener and cleanup daemons
     */
    void activate() {
      if(lthread == -1)
        pthread_create(&lthread, NULL, listen, this);
      else
        cout<<"listener already active!"<<endl;
      
      if(cthread == -1)
        pthread_create(&cthread, NULL, cleanup, this);
      else
        cout<<"cleanup thread already active!"<<endl;
    }

    /*
     * Terminate the daemons and stop the cloudlistener
     */
    void terminate() {
      TERM = true;
      cout<<"terminating cloudlistener."<<endl;
      pthread_join(lthread, NULL);
      pthread_join(cthread, NULL);
    }
};
