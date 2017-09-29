#pragma once

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

class Client {
public:
	Client(string host, int port);
	~Client();

	void run();

private:
	virtual void create();
	virtual void close_socket();
	void echo();
	bool send_request(string);
	string get_response();
	string read_message();
	string get_user_message(string, string);
	void response_to_put();
	void response_to_list();
	//void read_list_response(int);
	void response_to_read();
	void read_message_response(string, int);

	string host_;
	int port_;
	int server_;
	int buflen_;
	char* buf_;
	string cache;
};
