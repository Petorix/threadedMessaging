#include "messages.h"

Messages::Messages(mutex* m, condition_variable* cv1) {
	master = m;
	cv = cv1;
}

Messages::Messages() {};

void
Messages::addMessages(string name, Message mess) {
	unique_lock<mutex> lock(*master);
	mappedMessages[name].push_back(mess);
	lock.unlock();
}

map<string, vector<Message>>
Messages::getMessages() {
	//Do it this way so that the thread will spend even less time using a locked map.
	map<string, vector<Message>> copied_map = mappedMessages;
	return copied_map;
}

void
Messages::mapClear() {
	unique_lock<mutex> lock(*master);
	mappedMessages.clear();
	lock.unlock();
}
