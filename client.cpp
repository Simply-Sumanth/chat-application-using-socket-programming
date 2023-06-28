#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

using namespace std;

enum cli_status {start, idle, srv_res_wait, srv_res_processing}; // ended
enum transition {create, start_failure, connect_to_server, connection_failure, send_request, send_failure, got_response, response_time_out, get_response_failure, done};

class Client{
public:

	cli_status client_state;

	int cli_sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char request[256],response[256];

    bool connection;

	Client(char* host,char* port){
		this->server = gethostbyname(host);
		if(this->server == NULL){
			cout << "No such host" << endl;
			this->trigger(start_failure);
		}
		// cout << "gethost successful\n";
		this->portno = atoi(port);
	}

	int createSocket(){
		int sockfd = socket(AF_INET, SOCK_STREAM, 0);
		return sockfd;
	}

	void initialize(){

		this->cli_sockfd = createSocket();
		if(this->cli_sockfd < 0){
			cout << "Client was not assigned socket" << endl;
			this->trigger(start_failure);
		}
		// cout << "socket assigned\n";
        cout << "\n----------------------- Client is in 'Start' state -----------------------\n" << endl;

	}

	void connectToServer(){

		bzero((char *) &(serv_addr), sizeof(serv_addr));

	    serv_addr.sin_family = AF_INET;
	    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	    serv_addr.sin_port = htons(portno);

		if (connect(this->cli_sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
			cout << "Failed to connect client to server" << endl;
			this->trigger(connection_failure);
		}

		this->client_state = idle;

		cout << "             [ Connection established with the server ]" << endl;

       	cout << "\n----------------------- Client is in 'Idle' state -----------------------\n" << endl;
    	cout << "Start a conversation:" << endl;

    	// define talk_with_client() and go to rsponse waiting state in it

    	this->trigger(send_request);

	}


	void sendRequest(){
		bzero(this->request,255);

        if(this->client_state == idle || this->client_state == srv_res_processing){
            cout << "\n                                           > ";
            fgets(this->request,255,stdin);
        }

        int writing = write(this->cli_sockfd,this->request,strlen(this->request));  // sent request to server

        if(writing < 0)
        	this->trigger(send_failure);
        else if(strcmp(this->request,"quit\n") == 0){
			cout << "                 [ Connection terminated with the server ]" << endl;
        	this->trigger(done);
        }
        else{
        	this->client_state = srv_res_wait;
        	cout << "\n ------------- Client is in 'Server-response-waiting' state ------------ \n" << endl;
        	this->getResponse();
        }
	}


	void getResponse(){

		bzero(this->response,255);
        int reading = read(this->cli_sockfd,this->response,255);  // reading response from server

        if(reading < 0){
            cout << "Error while getting response from server..!" << endl;
            this->trigger(get_response_failure);
        }

	    cout << "\n ------------- Client is in 'Server-response-processing' state ------------ \n" << endl;

	    printf("\nServer: %s\n",response);

	    if(strcmp(this->response,"quit\n") == 0 || strcmp(this->response,"closed\n") == 0){
	    	this->quit("                 [ Server terminated connection ]");
	    }

        else{

	        this->client_state = srv_res_processing;

	        this->trigger(send_request);
        }

	}

	void trigger(transition tr){
		switch(this->client_state){

			case start:
				switch(tr){
					case create:
						this->initialize();
						this->client_state = start;
						break;

					case start_failure:
						this->quit("Could not inititate client");
						break;

					case connect_to_server:
						this->connectToServer();
						break;

					case connection_failure:
						this->quit("Unable to connect to server");
						break;

					default:
						// cout << "Blocked invalid transition from 'Start' state" << endl;
						break;
				}

				break;

			case idle:
				switch(tr){
					case send_request:
						sendRequest();
						break;

					case send_failure:
						this->quit("Failed to send request");
						break;

					case done:
						this->quit("Job done..!");
						break;

					default:
						// cout << "Blocked invalid transition from 'idle' state" << endl;
						break;
				}

			case srv_res_wait:
				switch(tr){
					case got_response:
						sendRequest();
						break;

					case get_response_failure:
						this->quit("Unable to get response from server");
						break;

					case response_time_out:
						break;

					default:
						// cout << "Blocked invalid transition from 'srv_res_wait' state" << endl;
						break;
				}
				
				break;

			case srv_res_processing:
				switch(tr){
					case send_request:
						sendRequest();
						break;

					case send_failure:
						this->quit("Failed to send request");
						break;

					case done:
						this->quit("Job done..!");
						break;

					default:
						// cout << "Blocked invalid transition from 'srv_res_processing' state" << endl;
						break;

				}
				
				break;
			
			default:
				cout << "Client went to invalid state. Something went wrong" << endl;
		}

	}

	void quit(string msg){
		cout << msg << endl;

		close(this->cli_sockfd);

        cout << "\n----------------------- Client reached 'End' state -----------------------\n" << endl;

	    cout << endl << "==============================================================================" << endl;

	    exit(0);
	}
};

int main(int argc,char* argv[]){

	if (argc != 3) {
       fprintf(stderr,"usage %s <hostname> <port>\n", argv[0]); // eg:  ./client localhost 9090
       exit(1);
    }

    // cout << "Got appropriate args\n";

	// cout << "Client() called\n";
	Client myClient = Client(argv[1],argv[2]);

	// cout << "create triggered\n";
	myClient.trigger(create);

	// cout << "connect_to_server triggered\n";
	myClient.trigger(connect_to_server);

	return 0;
}