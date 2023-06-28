/* A simple server in the internet domain using TCP. The port number is passed as an argument */

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

enum serv_status {start, listening, busy}; // ended state need not be implemented as no transitions would be there
enum transition {create,start_listen, start_failure, client_connection, connection_failure, time_out, quit_connection, done};	


class Server{

public:
	serv_status server_state;

	int serv_sockfd, cli_sockfd, portno;

	socklen_t clilen;

	char request[256],response[256];

	struct sockaddr_in serv_addr, cli_addr;


	Server(char* port){
		this->portno = atoi(port);
	}

	int createSocket(){
		int sockfd = socket(AF_INET, SOCK_STREAM, 0);
		return sockfd;
	}


	void makeReusable(){
		// helps in reusing same address and port again;	optional
		int opt = 1;
		if (setsockopt(this->serv_sockfd, SOL_SOCKET,
	                   SO_REUSEADDR | SO_REUSEPORT, &opt,
	                   sizeof(opt))) {
	        // perror("setsockopt");
			cout << "Failed to SETSOCKOPT" << endl;

			this->trigger(start_failure);
	        // exit(EXIT_FAILURE);
	    }

	}

	void initialize(){

		this->serv_sockfd = createSocket();
		if(this->serv_sockfd < 0){
			cout << "Server was not assigned socket" << endl;
			this->trigger(start_failure);
		}
		// cout << "socket assigned\n";

		this->makeReusable();
		// cout << "made reusable\n";

		bzero((char *) &serv_addr, sizeof(serv_addr));

		this->serv_addr.sin_family = AF_INET;
		this->serv_addr.sin_addr.s_addr = INADDR_ANY;
		this->serv_addr.sin_port = htons(portno);

		if (bind(this->serv_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {

			cout << "Failed to Bind socket with address" << endl;

			this->trigger(start_failure);
		}

		// cout << "bind successful\n";

	}

	void startListening(){
		// cout << "startListening() called\n";

		listen(serv_sockfd,3);
		// cout << "listen successful\n";
	}

	void startAccepting(){

		sleep(2);
        cout << "\n---------------------- Server is in 'Listening' state --------------------\n" << endl;


		this->clilen = sizeof(this->cli_addr);
		// cout << "clilen assigned\n";

		// wait for some time and proceed (check for time out)

		this->cli_sockfd = accept(serv_sockfd, (struct sockaddr *) &cli_addr, &clilen);

		// cout << "accept() called\n";

		if(this->cli_sockfd < 0){
			// cout << "Connection failure triggered\n";
			this->trigger(connection_failure);
		}

		cout << "             [ Connection established with a new client ]" << endl;

		this->server_state = busy;

        cout << "\n----------------------- Server is in 'Busy' state -----------------------\n" << endl;

		talk_with_client();

	}

	void talk_with_client(){
		bool client_status = true;
		do{
			bzero(this->request,256);

			int reading = read(this->cli_sockfd,this->request,255);

			if (reading < 0){
				cout << "ERROR reading from client" << endl;
				client_status = false;
				this->trigger(quit_connection);
			}
			
			printf("\nClient: %s\n",request);

			if(strcmp(this->request,"quit\n") == 0){
				client_status = false;
				this->trigger(quit_connection);
			}


			if(client_status == true){	// client has not said quit
				bzero(this->response,256);
				cout << "\n                                           > ";
				fgets(this->response,255,stdin);
			}

			int writing = write(this->cli_sockfd,this->response,strlen(this->response));

			if (writing < 0){
				cout << "ERROR writing to socket" << endl;
				client_status = false;
				this->trigger(quit_connection);
			}

			if(strcmp(this->response,"quit\n") == 0){	
				client_status = false;
				this->trigger(quit_connection);
			}

			else if(strcmp(this->response,"closed\n") == 0){
				client_status = false;
				close(this->cli_sockfd);
				this->trigger(done);
			}
			
		}while(client_status == true && this->server_state == busy);
	}


	void trigger(transition tr){

		switch(this->server_state){

			case start:
				switch(tr){
					case create:
						this->initialize();
						this->server_state = start;
        				cout << "\n----------------------- Server is in 'Start' state -----------------------\n" << endl;
						break;

					case start_listen:
						this->startListening();
						this->server_state = listening;
						this->trigger(client_connection);
						break;

					case start_failure:
						this->quit("Failed to create server");
						break;

					default:
						// cout << "Blocked invalid transition from start state" << endl;
						break;
				}

				break;

			case listening:
				switch(tr){
					case client_connection:
						this->startAccepting();
						break;

					case time_out:	// triggered isnside startAccepting() function before accept statement.. begin and wait for some specific time if its out, trigger time_out
						this->quit("Server stopped looking for connections : Time out");
						break;

					case connection_failure:	// triggered if cli_sockfd < 0 after accept statement
						this-> quit("Server could not accept connection from client");
						break;

					default:
						// cout << "Blocked invalid transition from listening state" << endl;
						break;

				}

				break;

			case busy:
				switch(tr) {
					case quit_connection:
						close(this->cli_sockfd);
						cout << "                 [ Connection terminated with the client ]" << endl;
						this->server_state = listening;
						this->trigger(client_connection);
						break;

					case done:
						this->quit("Job done...!");
						break;

					default:
						// cout << "Blocked invalid transition from busy state" << endl;
						break;
				}

				break;

			default:
				this->quit("Server went into invalid state");
		}

	}

	void quit(string msg){
		cout << msg << endl;

		// close server socket and client socket if exists
		close(this->serv_sockfd);
        
        cout << "\n----------------------- Server reached 'End' state -----------------------\n" << endl;

		cout << endl << "==============================================================================" << endl;

	    exit(0);

	}

};

int main(int argc,char* argv[]){

	if (argc != 2) {
		fprintf(stderr,"Usage: %s <port>\n",argv[0]);	// eg: ./server 9090
		exit(1);
	}
	// cout << "Got appropriate args\n";

	// cout << "Server() called\n";
	Server myServer = Server(argv[1]);

	// cout << "create triggered\n";
	myServer.trigger(create);

	// cout << "start_listen triggered\n";
	myServer.trigger(start_listen);


	return 0;
}