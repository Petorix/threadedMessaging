#pragma once

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <iterator>
#include <thread>
#include <mutex>
#include <string>
#include <queue>
#include <typeinfo>

#include "type_definitions.h"
#include "order_queue.h"
#include "messages.h"

class Server {
public:
	Server(int port, mutex*, mutex*, condition_variable*, condition_variable*, queue<Client>*, condition_variable*);
	~Server();

	void run();
	void handle(Client*, mutex*);
	OrderQueue queueReturn();
	OrderQueue* qqRR();
	int size;
	bool add;

private:
	void create();
	void close_socket();
	void serve();
	string get_request(Client*, mutex*);
	bool send_response(Client*, string, mutex*);
	string read_message(Client*);
	void handle_message(Client*, string, mutex*);
	string parse_message(Client*, string, mutex*);
	string read_put(Client*, int, mutex*);
	void store_message(string, string, string);
	string get_subjects(string);

	int port_;
	int server_;
	int buflen_;
	char* buf_;

	OrderQueue order_queue;
	Messages messagesss;
};
