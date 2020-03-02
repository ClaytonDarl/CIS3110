/* 
 * tcpserver.c - A simple TCP echo server 
 * usage: tcpserver <port>
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//Global variable for the parent socket
int parentfd; 

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

//Event signal handler provided from Giuliano to avoid port number not closing properly
void handle_signal(int err) {
    
    // global server socket parentfd
    close(parentfd);
    return;
}



/* Name: decipherRequest
 * Description: This function takes the request message from the client and determines if it is a valid GET, SUBMIT, or REMOVE request. 
 *              If the request is valid, it will call the appropriate function to access the Book Catalog, and return the success of the action
 *              to the user as a response message.
 * 
 * Parameter: request             The request message received from the client  
 * Parameter: childfd             The socket connection to the client
 * Return: None
*/ 
void decipherRequest(char request[], int childfd);


/* Name: parseRequest
 * Description: This function takes the request message from the client and breaks it apart to get all of the passed request tokens.
 *              The tokens are used by decipherRequest to determine the type of request, and if it's valid.
 * 
 * Parameter: request               The request message received from the client
 * Parameter: method                The method or header type i.e. METHOD, AUTHOR, TITLE
 * Parameter: value                 The method or header value i.e. GET, SUBMIT, REMOVE
 * Return: None
*/ 
void parseRequest(char request[], char method[], char value[]);



/* Name: getBooksByAuthor
 * Description: This function gets all of the Books for the given author from the Book Catalog, if there are any.
 * 
 * Parameter: bookAuthor            The author of the Books to retrieve
 * Parameter: childfd               The socket connection to the client
*/ 
void getBooksByAuthor(char bookAuthor[], int childfd);



/* Name: getBooksWithTitle
 * Description: This function gets all of the Books with the given title from the Book Catalog, if there are any.
 * 
 * Parameter: bookTitle             The title of the Books to retrieve
 * Parameter: childfd               The socket connection to the client
*/ 
void getBooksWithTitle(char bookTitle[], int childfd);



/* Name: getSpecificBook
 * Description: This function gets all of the locations of the specific Book Catalog, if it can be found.
 * 
 * Parameter: bookTitle             The title of the Books to retrieve
 * Parameter: bookAuthor            The author of the Books to retrieve
 * Parameter: childfd               The socket connection to the client
 * Return: None
*/ 
void getSpecificBook(char bookAuthor[], char bookTitle[], int childfd);



/* Name: launchClientLoop
 * Description: This function runs the client connection loop in a separate thread per client.
 * 
 * Parameter: fd                    The child socket ID of the client connecting
 * Return: NULL
 */ 
void* launchClientLoop(void* fd);



/* Name: removeBook
 * Description: This function takes the information for a Book to remove from the Book Catalog, if it exists within it.
 * 
 * Parameter: bookTitle         The title of the Book to submit
 * Parameter: bookAuthor        The author of the Book to submit
 * Parameter: bookLocation      The location of the Book to submit
 * Parameter: childfd           The socket connection to the client
 * Return: None
*/ 
void removeBook(char bookTitle[], char bookAuthor[], char bookLocation[], int childfd);



/* Name: submitBook
 * Description: This function takes the information for a Book from the client and adds it to the Book Catalog, if it is valid.
 *              This function will check for and disregard duplicate book submissions, reporting them to the client.
 * 
 * Parameter: bookTitle         The title of the Book to submit
 * Parameter: bookAuthor        The author of the Book to submit
 * Parameter: bookLocation      The location of the Book to submit
 * Parameter: childfd           The socket connection to the client
 * Return: None
*/ 
void submitBook(char bookTitle[], char bookAuthor[], char bookLocation[], int childfd);



//Main loop
int main(int argc, char **argv) {

    //The child socket number
    int childfd; 

    //The port number to listen on
    int portno;

    //The byte size of the client's address
    int clientlen;
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp; /* client host info */
    char *hostaddrp; /* dotted decimal host addr string */
    int optval; /* flag value for setsockopt */

    //Bind the CTRL+C shortcut to an event handler
    sigset(SIGINT, &handle_signal);
    sigset(SIGTERM, &handle_signal);


    //Verify the user provided a port number to connect to
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    //Get the port number from the command line
    portno = atoi(argv[1]);

    //Create the parent (server) socket
    parentfd = socket(AF_INET, SOCK_STREAM, 0);

    //If there was an error creating the parent socket, inform the user
    if (parentfd < 0) {
        error("ERROR opening socket.\n");
    }

    /* setsockopt: Handy debugging trick that lets 
    * us rerun the server immediately after we kill it; 
    * otherwise we have to wait about 20 secs. 
    * Eliminates "ERROR on binding: Address already in use" error. 
    */
    optval = 1;
    setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    //Build the server's internet address with an IP address and port number
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    //Bind the parent socket id to the port number
    if (bind(parentfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
        error("ERROR on binding");
    }

    //Listen for any connection requests, up to 5
    if (listen(parentfd, 5) < 0) {
        error("ERROR on listen");
    }

    clientlen = sizeof(clientaddr);
    
    //Main loop to wait for a connection request
    while (1) {

        //Wait for a client to connect
        childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
        
        //If the client's connection wasn't accepted, error
        if (childfd < 0) {
            error("ERROR on accept");
        }
        
        //Determine who the client is
        hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        
        //If something went wrong getting the host address
        if (hostp == NULL) {
        error("ERROR on gethostbyaddr");
        }

        //Get the client's IP address
        hostaddrp = inet_ntoa(clientaddr.sin_addr);
        
        //If there was an error getting the client's IP address inform the user
        if (hostaddrp == NULL) {
            error("ERROR on inet_ntoa\n");
        }

        //If the connection was successfully made, inform the user
        printf("server established connection with %s (%s)\n", 
        hostp->h_name, hostaddrp);

        //Create a thread for the client connection
        pthread_t clientThread;

        //Bind the main loop function to the thread and pass the child fd
        pthread_create(&clientThread, NULL, launchClientLoop, (void*) &childfd);

    }

    return 0;
}



//THIS IS THE MAIN LOOP FOR EACH CLIENT THREAD, READ A REQUEST, PARSEREQUEST() and DECIPHERREQUEST() THEN CALL SPECIFIC FUNCTION WITHIN DECIPHERREQUEST()
//FUNCTION launchClientLoop
void* launchClientLoop(void* fd) {

    //Get the client's socket ID (Responses are only returned to this client)
    int childfd = *((int*)fd);

    //(Testing purposes)
    printf("Child has connected.\n");
    printf("FD: %d\n", (int)childfd);

    //Loop to read client requests until they disconnect
    while (childfd >= 0) {
    
        //The request message from the client
        char requestMessage[250] = ""; 

        //The length of the client request message
        int requestLength; 

        //Read the client's request
        requestLength = read(childfd, requestMessage, 250);
        
        //If there was an error reading the client's request, inform the user
        if (requestLength < 0) {
            error("ERROR reading from socket");
        }

        //Else if the client disconnects, break out of the loop
        else if (requestLength == 0) {
            printf("Client disconnect.\n");
            break;
        }

        //Else a request was successfully received
        else {

            //Make sure the request is terminated properly with a LF character
            if (requestMessage[requestLength-1] == '\n') {
                requestMessage[requestLength-1] = '\0';     

                //Find out what the request was         
                decipherRequest(requestMessage, childfd);
            }

            //Otherwise, send an error message back to the user
            else {
                write(childfd, "404:BAD REQUEST,MESSAGE:Request Message is missing ending newline character.\n", 78);
            }
        }
    }

    return NULL;
}



//FUNCTION decipherRequest 
void decipherRequest(char request[], int childfd) {

    //Containers for the type and value of the request header line i.e. 'METHOD:GET' or 'METHOD:REMOVE'
    char requestHeaderType[15];
    char requestHeaderValue[15];

    //Containers for the type and value of a request line i.e. 'AUTHOR:Clayton' or 'TITLE:Networking'
    char requestMethodType[15];
    char requestMethodValue[100];

    //Containers for the Book's title, author, and location data
    char requestTitle[100];
    char requestAuthor[50];
    char requestLocation[50];

    //Parse the request message to see what type of request this is
    parseRequest(request, requestHeaderType, requestHeaderValue);

    //SUBMIT REQUEST
    if (strcmp(requestHeaderValue, "SUBMIT") == 0) {

        //Get the Book's Title and clear the method type
        parseRequest(request, requestMethodType, requestTitle);
        requestMethodType[0] = '\0';

        //Get the Book's Author and clear the method type
        parseRequest(request, requestMethodType, requestAuthor);
        requestMethodType[0] = '\0';

        //Get the Book's Location and clear the method type
        parseRequest(request, requestMethodType, requestLocation);
        requestMethodType[0] = '\0';

        //CALL THE SPECIFIC SUBMIT FUNCTION
        submitBook(requestTitle, requestAuthor, requestLocation, childfd);
    }

    //GET REQUEST
    else if (strcmp(requestHeaderValue, "GET") == 0) {

        //Parse the request for the first METHOD field and value
        parseRequest(request, requestMethodType, requestMethodValue);

        //If the METHOD field is "AUTHOR", this is a GET Request for books by an AUTHOR
        if (strcmp(requestMethodType, "AUTHOR") == 0) {

            //Copy the Book's author
            strcpy(requestAuthor, requestMethodValue);

            //CALL THE SPECIFIC GET BOOKS BY AUTHOR FUNCTION
            getBooksByAuthor(requestAuthor, childfd);
        }

        //Else if the METHOD field is "TITLE"
        else if (strcmp(requestMethodType, "TITLE") == 0) {

            //Copy the Book's title
            strcpy(requestTitle, requestMethodValue);

            //Clear the requestMethodType and requestMethodValue
            requestMethodType[0] = '\0';
            requestMethodValue[0] = '\0';

            //Check for an "AUTHOR" field
            parseRequest(request, requestMethodType, requestMethodValue);

            //If the request has an "AUTHOR" field
            if (strcmp(requestMethodType, "AUTHOR") == 0) {

                //Copy the Book's author
                strcpy(requestAuthor, requestMethodValue);

                //CALL THE SPECIFIC GET BOOK WITH AUTHOR AND TITLE FUNCTION
                getSpecificBook(requestTitle, requestAuthor, childfd);
            }

            //Else the request doesn't have an AUTHOR field
            else {

                //CALL THE SPECIFIC GET BOOKS BY TITLE FUNCTION
                getBooksWithTitle(requestTitle, childfd);
            }
        }

        //Else the METHOD field is invalid
        else {
            
            //Inform the user that their request was invalid
            int responseLength = write(childfd, "404:BAD REQUEST,MESSAGE:Request Message has an invalid field.", 62);

            if (responseLength < 0) {
                error("ERROR writing to socket");
            }
        }
    }

    //REMOVE REQUEST
    else if (strcmp(requestHeaderValue, "REMOVE") == 0) {
        
        //Get the Book's Title and clear the method type
        parseRequest(request, requestMethodType, requestTitle);
        requestMethodType[0] = '\0';

        //Get the Book's Author and clear the method type
        parseRequest(request, requestMethodType, requestAuthor);
        requestMethodType[0] = '\0';

        //Get the Book's Location and clear the method type
        parseRequest(request, requestMethodType, requestLocation);
        requestMethodType[0] = '\0';

        //CALL THE SPECIFIC REMOVE BOOK FUNCTION
        removeBook(requestTitle, requestAuthor, requestLocation, childfd);
    }

    //INVALID REQUEST (Needs to be turned into a WRITE ERROR EVENTUALLY)
    else {
        printf("Invalid request type.\n");
    }
}



//FUNCTION parseRequest
void parseRequest(char requestMessage[], char method[], char value[]) {

    //The length of the request message passed
    int requestLength = strlen(requestMessage);

    //The length of the HEADER:VALUE or METHOD:VALUE token
    int tokenLength = 0;

    //The length of the HEADER or METHOD i.e. 'METHOD', 'TITLE', 'AUTHOR'
    int methodLength = 0;

    //The length of the VALUE for a given HEADER or METHOD i.e. 'GET', 'SUBMIT', 'Book Title'
    int valueLength = 0;

    //This will hold a copy of the request message to truncate the original
    char newRequestMessage[300];

    //If the passed request message is blank, set the method and value to be blank and return
    if (strlen(requestMessage) == 0) {
        strcpy(method, "");
        strcpy(value, "");
        return;
    }

    //Loop through the original message string and get the length of the method's header value
    for (int i = 0; i < requestLength; i++) {

        //Stop at the delimitter
        if (requestMessage[i] == ':') {
            break;
        }

        //Increment the length
        else {
            methodLength++;
        }
    }

    //Copy the method's header from the request message
    strncpy(method, requestMessage, methodLength);
    method[methodLength] = '\0';

    //Loop through the original message string and get the length of the method's value
    for (int j = methodLength + 1; j < requestLength; j++) {

        //Stop at the delimitter
        if (requestMessage[j] == ',') {
            break;
        }

        //Increment the length
        else {
            valueLength++;
        }
    }

    //Copy the method's value from the string
    strncpy(value, requestMessage + methodLength + 1, valueLength);
    value[valueLength] = '\0';

    //Calculate the length of the method + the value and + 2 for the delimitters
    tokenLength = methodLength + valueLength + 2;

    //If there are still more tokens to parse, we will need to keep them
    if (requestLength - tokenLength > 0) {

        //Make a cropped copy of the original string including only the characters after the parsed token
        strncpy(newRequestMessage, requestMessage + tokenLength, requestLength - tokenLength);
        newRequestMessage[requestLength - tokenLength] = '\0';

        //Clear the original message string
        requestMessage[0] = '\0';

        //Copy the string's copy into the original string
        strcpy(requestMessage, newRequestMessage);
        requestMessage[requestLength-1] = '\0';
    }

    //We're done parsing the request message, so it can be fully erased
    else {
        requestMessage[0] = '\0';
    }
}



//FUNCTION getBooksByAuthor
void getBooksByAuthor(char bookAuthor[], int childfd) {

    int responseLength;

    //Call the function to get the Books by this author
    responseLength = write(childfd, "GET BY AUTHOR\n", 15);

    if (responseLength < 0) {
        error("ERROR writing to socket");
    }
}



//FUNCTION getBooksWithTitle
void getBooksWithTitle(char bookTitle[], int childfd) {

    int responseLength;

    //Call the function to get the Books with this title
    responseLength = write(childfd, "GET WITH TITLE\n", 16);

    if (responseLength < 0) {
        error("ERROR writing to socket");
    }
}



//FUNCTION getSpecificBook
void getSpecificBook(char bookTitle[], char bookAuthor[], int childfd) {

    int responseLength;

    //Call the function to get the locations of the specific Book
    responseLength = write(childfd, "GET SPECIFIC\n", 14);

    if (responseLength < 0) {
        error("ERROR writing to socket");
    }
}



//FUNCTION removeBook
void removeBook(char bookTitle[], char bookAuthor[], char bookLocation[], int childfd) {
    
    int responseLength;
    
    responseLength = write(childfd, "203: Removed.\n", 15);

    if (responseLength < 0) {
        error("ERROR writing to socket");
    }
}



//FUNCTION submitBook
void submitBook(char bookTitle[], char bookAuthor[], char bookLocation[], int childfd) {
    
    int responseLength;
    
    responseLength = write(childfd, "203: Submitted.\n", 17);

    if (responseLength < 0) {
        error("ERROR writing to socket");
    }
}

