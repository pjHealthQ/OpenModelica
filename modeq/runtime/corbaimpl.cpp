#include "modeq_communication.h"
#include "modeq_communication_impl.h"



extern "C" {
#include "rml.h"
#include "../values.h"
#include <stdio.h>
#include "../absyn_builder/yacclib.h"
#include <pthread.h>
}

#include <cstdlib>
#include <iostream>
#include <fstream>

using namespace std;

pthread_mutex_t lock;

// Condition variable for keeping modeq waiting for client requests
pthread_cond_t modeq_waitformsg;
pthread_mutex_t modeq_waitlock;
bool modeq_waiting=false;

// Condition variable for keeping corba waiting for returnvalue from modeq
pthread_cond_t corba_waitformsg;
pthread_mutex_t corba_waitlock;
bool corba_waiting=false;

char * modeq_message;


CORBA::ORB_var orb;
CORBA::BOA_var boa;

ModeqCommunication_impl * server;

extern "C" {
void writeServerId(CORBA::ORB_var orb);
void* runOrb(void*arg);
  
void Corba_5finit(void)
{

}

RML_BEGIN_LABEL(Corba__initialize)
{
  char *dummyArgv="modeq";
  int zero=0;
  pthread_cond_init(&modeq_waitformsg,NULL);
  pthread_cond_init(&corba_waitformsg,NULL);
  pthread_mutex_init(&corba_waitlock,NULL);
  pthread_mutex_init(&modeq_waitlock,NULL);
  
  orb = CORBA::ORB_init(zero, 0,"mico-local-orb");
  boa = orb->BOA_init(zero,0,"mico-local-boa");
  
  server = new ModeqCommunication_impl(); 

  writeServerId(orb);
  boa->impl_is_ready(CORBA::ImplementationDef::_nil());

  // Start thread that listens on incomming messages.
  pthread_t orb_thr_id;
  if( pthread_create(&orb_thr_id,NULL,&runOrb,NULL)) {
    cerr << "Error creating thread for corba communication." << endl;
    RML_TAILCALLK(rmlFC);
  }
  
  std::cout << "Created server." << std::endl;
  RML_TAILCALLK(rmlSC);
}
RML_END_LABEL

void* runOrb(void* arg) 
{
  orb->run();
  CORBA::release(server);
  return NULL;
}

void writeServerId(CORBA::ORB_var orb)
{
  ofstream tmpFile ("/tmp/openmodelica.objid");  // Should be different on Windows
  CORBA::String_var ref = orb->object_to_string(server);
  tmpFile << ref << endl;
  tmpFile.close();
}

RML_BEGIN_LABEL(Corba__wait_5ffor_5fcommand)
{
  pthread_mutex_lock(&modeq_waitlock);
  while (!modeq_waiting) {
    pthread_cond_wait(&modeq_waitformsg,&modeq_waitlock);
  }
  modeq_waiting = false;
  pthread_mutex_unlock(&modeq_waitlock);

  rmlA0=mk_scon(modeq_message);
  pthread_mutex_lock(&lock); // Lock so no other tread can talk to modeq.
  RML_TAILCALLK(rmlSC);
}
RML_END_LABEL

RML_BEGIN_LABEL(Corba__sendreply)
{
  char *msg=RML_STRINGDATA(rmlA0);

  // Signal to Corba that it can return, taking the value in message
  pthread_mutex_lock(&corba_waitlock); 
  corba_waiting=true;
  modeq_message =CORBA::string_dup(msg);

  pthread_cond_signal(&corba_waitformsg);
  pthread_mutex_unlock(&corba_waitlock);

  pthread_mutex_unlock(&lock); // Unlock, so other threads can ask modeq stuff.
  RML_TAILCALLK(rmlSC);
}
RML_END_LABEL

RML_BEGIN_LABEL(Corba__close)
{
  boa->deactivate_impl(CORBA::ImplementationDef::_nil());
  orb->shutdown(TRUE);
  pthread_yield(); // Allowing other thread to shutdown.
  RML_TAILCALLK(rmlSC);
}
RML_END_LABEL
}
