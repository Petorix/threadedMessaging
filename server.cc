#include "server.h"

OrderQueue*
Server::qqRR() {
	return &order_queue;
}

OrderQueue
Server::queueReturn() {
	return order_queue;
}

void
worker_start(mutex* m, Server* server) {
	while(1){
	Client current_client = server->queueReturn().Get();
	server->handle(&current_client, m);

	if(current_client.open == false) {
		close(current_client.id);
	}
	else {
		server->queueReturn().Add(current_client);
	}
}
}

int
main(int argc, char **argv)
{
	vector<thread> workers;
	int option, port;
	mutex mMapAccess, mQueueAccess, m;
	condition_variable cvMap, cvQueue, addCV;
	queue<Client> cQ;

	// setup default arguments
	port = 5000;

	// see "man 3 getopt"
	while ((option = getopt(argc, argv, "p:")) != -1) {
		switch (option) {
		case 'p':
			port = atoi(optarg);
			break;
		default:
			cout << "server [-p port]" << endl;
			exit(EXIT_FAILURE);
		}
	}
	Server server = Server(port, &mMapAccess, &mQueueAccess, &cvMap, &cvQueue, &cQ, &addCV);

	int THREAD_COUNT = 10;
	for(int i = 0; i < THREAD_COUNT; i++) {
		workers.push_back(thread(worker_start, &m, &server));
	}

//	They will be using a thread safe class called OrderQueue to make sure that they don't use data simultaneously

	server.run();
}
Server::Server(int port, mutex* mM, mutex* mQ, condition_variable* cvM, condition_variable* cvQ, queue<Client>* cQ, condition_variable* addCV) {
	// setup variables
	port_ = port;
	buflen_ = 1024;
	buf_ = new char[buflen_ + 1];

	size = 0;
	add = true;

	order_queue = OrderQueue(mQ, cvQ, &size, cQ, &add, addCV);
	messagesss = Messages(mM, cvM);
}

Server::~Server() {
	delete buf_;
}

void
Server::run() {
	// create and run the server
	create();
	serve();
}

void
Server::create() {
	struct sockaddr_in server_addr;

	// setup socket address structure
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	// create socket
	server_ = socket(PF_INET, SOCK_STREAM, 0);
	if (!server_) {
		perror("socket");
		exit(-1);
	}

	// set socket to immediately reuse port when the application closes
	int reuse = 1;
	if (setsockopt(server_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		perror("setsockopt");
		exit(-1);
	}

	// call bind to associate the socket with our local address and
	// port
	if (bind(server_, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind");
		exit(-1);
	}

	// convert the socket to listen for incoming connections
	if (listen(server_, SOMAXCONN) < 0) {
		perror("listen");
		exit(-1);
	}
}

void
Server::close_socket() {
	close(server_);
}

void
Server::serve() {
	// setup client

	int cc;
	struct sockaddr_in client_addr;
	socklen_t clientlen = sizeof(client_addr);

	//Treat this loop as the "Head Thread" which will accept clients and place them within a queue
	//TODO: Make sure it doesn't leave the while loop prematurely

	while ((cc = accept(server_, (struct sockaddr *)&client_addr, &clientlen)) > 0) {

		//handle(client);
		Client client;
		client.id = cc;
		client.cache = "";
		client.open = true;

		order_queue.Add(client);
	}
	close_socket();
}

void
Server::handle(Client* client, mutex* mutex) {
		string request = get_request(client, mutex);

		if (request.empty()) {
			client->open = false;
			return;
		}
		handle_message(client, request, mutex);
}

void
Server::handle_message(Client* client, string message, mutex* mutex) {
	string response = parse_message(client, message, mutex);
	send_response(client, response, mutex);
}

string
Server::parse_message(Client* client, string message, mutex* mutex) {

	stringstream ss;
	string word = "";
	ss << message;
	ss >> word;

	string name, subject, data, response;
	int length, index;

	if (word == "reset") {
		messagesss.mapClear();
		return "OK\n";
	}

	if (word == "put") {

		ss >> name;
		ss >> subject;
		ss >> length;
		if (ss.fail()){
			return "error invalid message: put\n";
		}
		data = read_put(client, length, mutex);
		if (data == "") {
			return "error could not read entire message\n";
		}
		store_message(name, subject, data);
		return "OK\n";
	}

	if (word == "list") {

		ss >> name;

		if (ss.fail()){
			return "error invalid message: list\n";
		}

		//subject = get_subjects(name);
		subject = "";
		map<string, vector<Message>> current_map = messagesss.getMessages();
		for (int i = 0; i < current_map.at(name).size(); i++)
		{
			subject += to_string(i+1) + " " + current_map.at(name)[i].subject+ "\n";
		}
		response = "list "+to_string(current_map.at(name).size())+"\n";
		response += subject;
		return response;
	}

	if (word == "get") {

		ss >> name;
		ss >> index;

		if (ss.fail()){
			return "error invalid message: get\n";
		}

		map<string, vector<Message>> current_map = messagesss.getMessages();
		if (index <= 0 || current_map.find(name) == current_map.end() || index > current_map.at(name).size()){
			return "error invalid message\n";
		}
		index -= 1;
		// Copied this from get_subjects(string)
		for (int i = 0; i < current_map.at(name).size(); i++) {
			if (i == index) {
				subject = current_map.at(name)[index].subject;
				data = current_map.at(name)[index].data;
			}
		}

		response = "message "+ subject+" "+to_string(data.length())+"\n";
		response += data;
		return response;
	}
	return "error invalid message\n";
}

// Storing and getting messages and subjects //
void
Server::store_message(string name, string subject, string data) {

	Message m;
	m.subject = subject;
	m.data = data;
	messagesss.addMessages(name, m);
}

string
Server::get_subjects(string name) {
	string response = "";
	int counter = 0;
	map<string, vector<Message>> current_map = messagesss.getMessages();
	if (current_map.find(name) == current_map.end()) {
		return response;
	}

	for (int i = 0; i < current_map.at(name).size(); i++)
	{
		response += to_string(i+1) + " " + current_map.at(name)[i].subject+ "\n";
	}
	return response;
}


// General Handling
string
Server::get_request(Client* client, mutex* m) {
	string message = "";

		while (true) {
			if(client->cache == "") {
			//unique_lock<mutex> lock(*m);
			int nread = recv(client->id, buf_, 1024, 0);
			//lock.unlock();
			if (nread < 0) {
				if (errno == EINTR)
					// the socket call was interrupted -- try again
					continue;
				else
					// an error occurred, so break out
					return "";
			}
			else if (nread == 0) {
				// the socket is closed
				return "";
			}
			// be sure to use append in case we have binary data
			client->cache.append(buf_, nread);
			}
			message = read_message(client);
			if (message == "") {
				continue;
			}
			else {
				return message;
			}

		}
}

string
Server::read_message(Client* client) {
	stringstream ss(client->cache);
	string returnMessage = "";
	string updatedCache = "";
	string line = "";
	getline(ss, returnMessage);
	while (getline(ss, line)) {
		if(line.find_first_not_of(' ') != std::string::npos) {
		line += "\n";
		updatedCache += line;
		}

	}
	client->cache = updatedCache;

	return returnMessage;
}


// Send Responses
bool
Server::send_response(Client* client, string response, mutex* m) {
	// prepare to send response
	const char* ptr = response.c_str();
	int nleft = response.length();
	int nwritten;
	// loop to be sure it is all sent
	while (nleft) {
		//TODO: Does this need to be thread-safe too?
		//unique_lock<mutex> lock(*m);
		nwritten = send(client->id, ptr, nleft, 0);
		//lock.unlock();
		if ((nwritten < 0)) {
			if (errno == EINTR) {
				// the socket call was interrupted -- try again
				continue;
			}
			else {
				// an error occurred, so break out
				perror("write");
				return false;
			}
		}
		else if (nwritten == 0) {
			// the socket is closed
			return false;
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return true;
}


// Handling 'put' NOT 'read' xD
string
Server::read_put(Client* client, int length, mutex* m) {
	string data = client->cache;
	if (data.size() != length && iswspace(data[data.length()-1])) data = data.substr(0,data.length()-1);
	while (data.size() < length) {
		//unique_lock<mutex> lock(*m);
		int nread = recv(client->id, buf_, 1024, 0);
		//lock.unlock();
		if (nread < 0) {
			if (errno == EINTR)
			{
				// the socket call was interrupted -- try again
				continue;
			}
			else{
				// an error occurred, so break out
				return "";
			}
		}
		else if (nread == 0) {
			// the socket is closed
			return "";
		}
		data.append(buf_, nread);
	}

	if (data.length() > length) {
		client->cache = data.substr(length);
		data = data.substr(0, length);
	}
	else {
		client->cache = "";
	}


	return data;

}
