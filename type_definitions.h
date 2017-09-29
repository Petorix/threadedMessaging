#pragma once

using namespace std;

struct Message{
	string subject;
	string data;
};

struct Client {
	int id;
	string cache;
	bool open;
};
