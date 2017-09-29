#include "order_queue.h"

OrderQueue::OrderQueue(mutex* m, condition_variable* cv1, int* sizeT, queue<Client>* cQ, bool* a, condition_variable* cv2) {
  mom = m;
  cv = cv1;
  addCV = cv2;
  size = sizeT;

  clientQueue = cQ;

  add = a;
}

OrderQueue::OrderQueue() {};

void
OrderQueue::Add(Client cockadoodle) {

  unique_lock<mutex> lock(*mom);
  while(not *add) {
    addCV->wait(lock);
  }
  if(cockadoodle.open != false) {
    clientQueue->push(cockadoodle);
    (*size)++;
    cv->notify_one();
  }
  lock.unlock();
}

Client
OrderQueue::Get() {
  unique_lock<mutex> lock(*mom);
  while(transInt() == 0) {
    cv->wait(lock);
  }
  *add = false;
  Client c = (*clientQueue).front();
  clientQueue->pop();
  (*size)--;
  addCV->notify_one();
  *add = true;
  lock.unlock();
  return c;
}

queue<Client>
OrderQueue::getQueue() {
    return *clientQueue;
}

queue<Client>*
OrderQueue::getQQ() {
  return clientQueue;
}

int
OrderQueue::transInt() {
  return *size;
}
