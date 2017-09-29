#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <iostream>
#include <typeinfo>

#include "type_definitions.h"

class OrderQueue {
public:
  OrderQueue(mutex*, condition_variable*, int*, queue<Client>*, bool*, condition_variable*);
  OrderQueue();
  ~OrderQueue(){};

void Add(Client);
Client Get();
queue<Client> getQueue();
queue<Client>* getQQ();
int transInt();

int* size;
bool* add;

private:
queue<Client>* clientQueue;
mutex* mom;
condition_variable* cv;
condition_variable* addCV;

};
