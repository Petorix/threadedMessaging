#pragma once

#include <map>
#include <mutex>
#include <vector>
#include <condition_variable>

#include "type_definitions.h"

class Messages {
public:
  Messages(mutex*, condition_variable*);
  Messages();
  ~Messages() {};

  void addMessages(string, Message);
  map<string, vector<Message>> getMessages();
  void mapClear();

private:
  mutex* master;
  condition_variable* cv;
  map<string, vector<Message>> mappedMessages;
};
