#include "client.h"

int main(int argc, char **argv) {
	int option;

	int port = 5000;
	string host = "localhost";

	while ((option = getopt(argc, argv, "s:p:")) != -1) {
		switch (option) {
		case 'p':
			port = atoi(optarg);
			break;
		case 's':
			host = optarg;
			break;
		default:
			cout << "client [-s server] [-p port]" << endl;
			exit(EXIT_FAILURE);
		}
	}

	Client client = Client(host, port);
	client.run();
}

Client::Client(string host, int port) {
	// setup variables
	host_ = host;
	port_ = port;
	buflen_ = 1024;
	buf_ = new char[buflen_ + 1];
	cache = "";
}

Client::~Client() {
}

void Client::run() {
	// connect to the server and run echo program
	create();
	echo();
}

void
Client::create() {
	struct sockaddr_in server_addr;

	// use DNS to get IP address
	struct hostent *hostEntry;
	hostEntry = gethostbyname(host_.c_str());
	if (!hostEntry) {
		cout << "No such host name: " << host_ << endl;
		exit(-1);
	}

	// setup socket address structure
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_);
	memcpy(&server_addr.sin_addr, hostEntry->h_addr_list[0], hostEntry->h_length);

	// create socket
	server_ = socket(PF_INET, SOCK_STREAM, 0);
	if (!server_) {
		perror("socket");
		exit(-1);
	}

	// connect to server
	if (connect(server_, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("connect");
		exit(-1);
	}
}

void
Client::close_socket() {
	close(server_);
}

void
Client::echo() {
	string line;

	// loop to handle user interface
	while (true) {
		stringstream ss;
		cout << "% ";
		getline(cin, line);
		ss << line;
		string word;
		ss >> word;

		if (word == "send")
		{
			string user, subject, returnLine;
			ss >> user >> subject;
			returnLine = get_user_message(user, subject);

			bool success = send_request(returnLine);
			if (not success)
				break;
			response_to_put();
		}
		else if (word == "list")
		{
			string user, returnLine;
			ss >> user;
			returnLine = "list "+user+"\n";

			bool success = send_request(returnLine);
			if (not success)
				break;
			response_to_list();
		}
		else if (word == "read")
		{
			string user, returnLine;
			int index;
			ss >> user >> index;
			returnLine = "get "+user+" "+to_string(index)+"\n";

			bool success = send_request(returnLine);
			if (not success)
				break;
			response_to_read();
		}

		else if (word == "quit")
		{
			return;
		}
		else{
			cout << "Error, Invalid Message\n";
		}
	}
	cout << "closing socket" << endl;
	close_socket();
}



bool
Client::send_request(string request) {
	// prepare to send request
	const char* ptr = request.c_str();
	int nleft = request.length();
	int nwritten;
	// loop to be sure it is all sent
	while (nleft) {
		if ((nwritten = send(server_, ptr, nleft, 0)) < 0) {
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


// General Handling


// Returns FIRST LINE of server response, stores rest in CACHE
string
Client::get_response() {
	string message = "";

	if (cache == "") {
		// read until we get a newline
		while (true) {
			int nread = recv(server_, buf_, 1024, 0);
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
			cache.append(buf_, nread);
			message = read_message();
			if (message == "") {
				continue;
			}
			else {
				return message;
			}

		}
	}
	else {
		return read_message();
	}

}

// Grabs the first line from the cache and returns that data 
string
Client::read_message() {
	stringstream ss(cache);
	string returnMessage = "";
	string updatedCache = "";
	string line = "";
	getline(ss, returnMessage);
	while (getline(ss, line)) {
		line += "\n";
		updatedCache += line;
	}
	cache = updatedCache;
	return returnMessage;
}



// I did it this way because the server sends a response after each request

// Handling Send
void
Client::response_to_put() {
	string message = get_response();
	
	if (message != "OK") {
		cout << "Server returned bad message: " << message;
	}
}

string
Client::get_user_message(string user, string subject) {

	cout << "- Type your message. End with a blank line - \n";
	string message = "";
	string line = "";

	while (getline(cin, line)) {
		line += "\n";
		if (line == "\n") {
			message = "put "+user+" "+subject+" "+to_string(message.size())+"\n"+message;
			return message;
		}
		message += line;
		line.clear();
	}
}


// Handling List
void
Client::response_to_list() {
	string message = get_response();
	stringstream ss(message);
	string parsedMessage = "";
	ss >> parsedMessage;
	int num;

	if (parsedMessage != "list") {
		cout << "Server returned bad message: " << message;
		return;
	}
	ss >> num;

	int total = 0;
	string data;
	while (total < num) {
		data = read_message();
		if (data == "")
		{
			cout << "List length doesn't match server response" << endl;
		}
		//getline(ss, data);
		cout << data << endl;
		total++;
	}
}


// Handling Read
void
Client::response_to_read() {
	string message = get_response();
	stringstream ss(message);
	string parsedMessage = "";
	string subject = "";
	ss >> parsedMessage;
	int length;

	if (parsedMessage != "message") {
		cout << "Server returned bad message: " << message;
	}
	
	ss >> subject;
	ss >> length;
	read_message_response(subject, length);
}

void
Client::read_message_response(string subject, int length){
	string data = cache;

	// If the cache given wasn't big enough to grab the rest of the data
	// then repeat until you grab enough data
	while (data.length() < length) {
		int nread = recv(server_, buf_, 1024, 0);
		if (nread < 0) {
			if (errno == EINTR)
			{
				// the socket call was interrupted -- try again
				continue;
			}
			else{
				// an error occurred, so break out
				cout << "Server did not send the whole message: " + data;
				return;
			}
		}
		else if (nread == 0) {
			// the socket is closed
			cout << "the socket is closed" << endl;
			return;
		}
		data.append(buf_, nread);
	}

	if (data.length() > length) {
		cache = data.substr(length);
		data = data.substr(0, length);
	}
	else {
		cache = "";
	}
	cout << subject << endl << data;
}
