#define _XOPEN_SOURCE 500

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>

//A doubly linked list representing a Book catalog
typedef struct book {
    char title[100];
    char author[100];
    char location[100];

    struct book* previous;
    struct book* next;
} Book;

//The global book catalog to be accessed by all clients simultaneously
Book* bookCatalog = NULL;

//Global variable for the parent socket
int parentfd; 

//The semaphore variable to ensure process synchronization
sem_t mutex;



/* Name: decipherRequest
 * Description: This function takes the request message from the client and determines if it is a valid GET, SUBMIT, or REMOVE request. 
 *              If the request is valid, it will call the appropriate function to access the Book Catalog, and return the success of the action
 *              to the user as a response message.
 * 
 * Parameter: request               The request message received from the client  
 * Parameter: childfd               The socket connection to the client
 * Return: None
*/ 
void decipherRequest(char request[], int childfd);



/* Name: getBooksByAuthor
 * Description: This function attempts to GET all of the Books with the matching author specified in the user's request.
 *              If no books were found, a NOT FOUND response message will be returned, otherwise, a response with all of the associated Books
 *              will be returned.
 *
 * Parameter: head                  The head pointer to the Book Catalog
 * Parameter: author                The name of the author to search for Book matches with
 * Parameter: childfd               The socket connection to the client
 * Return: None
*/
void getBooksByAuthor(Book* head, char author[100], int childfd);



/* Name: getBooksWithTitle
 * Description: This function attempts to GET all of the Books with the matching title specified in the user's request. 
 *              If no Books were found, a NOT FOUND response message will be returned, otherwise a response with all of the associated Books
 *              will be returned.
 * 
 * Parameter: head                  The head pointer to the Book Catalog
 * Parameter: title                 The name of the title to search for Book matches with
 * Parameter: childfd               The socket connection to the client
 * Return: None
*/
void getBooksWithTitle(Book* head, char title[100], int childfd);



/* Name: getSpecificBook
 * Description: This function attemps to GET all of the locations of the Book with the matching title and author specified by the user's request.
 *              If no Books were found, a NOT FOUND response message will be returned, otherwise a response with all of the associated Book locations
 *              will be returned.
 * 
 * Parameter: head                  The head pointer to the Book Catalog
 * Parameter: title                 The name of the title to search for Book matches with
 * Parameter: author                The name of the author to search for Book matches with
 * Parameter: childfd               The socket connection to the client
 * Return: None
*/
void getSpecificBook(Book* head, char title[100], char author[100], int childfd);



/* Name: handleServerClose
 * Description: Event signal handler provided to avoid port number not closing properly
 * 
 * Return: None
*/ 
void handleServerClose();



/* Name: launchClientLoop
 * Description: This function runs the client connection loop in a separate thread per client.
 * 
 * Parameter: fd                    The child socket fd of the client connected
 * Return: NULL
*/ 
void* launchClientLoop(void* fd);



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



/* Name: removeAllBooks
 * Description: This function removes all of the Books from the Catalog. This function is called when the server is killed.
 *
 * Parameter: head                  A pointer to the Book Catalog's head
 * Return: None
*/
void removeAllBooks(Book** head);



/* Name: removeBook
 * Description: This function attempts to remove the Book with the given information from the Catalog. If the Book doesn't exist within the catalog,
 *              a NOT FOUND response message is returned. Otherwise, a REMOVED response message is returned.
 * 
 * Parameter: head                  A pointer to the Book Catalog's head
 * Parameter: title                 The title of the Book to remove
 * Parameter: author                The name of the author of the Book to remove
 * Parameter: location              The location of the Book to remove
 * Parameter: childfd               The socket connection to the client
 * Return: None
*/ 
void removeBook(Book** head, char author[100], char title[100], char location[100], int childfd);



/* Name: sendServerResponse
 * Description: This function attempts to send the passed server response to the client socket specified by childfd.
 *              If unsuccessful, an error message will be reported on the server.
 * 
 * Parameter: childfd           The socket fd of the client connection
 * Parameter: response          The server response to send to the client
 * Parameter: length            The length of the response to send to the client
 * Return: None
*/ 
void sendServerResponse(int childfd, char response[100], int length);



/* Name: submitBook
 * Description: This function attempts to submit the Book with the given information to the Catalog. 
 *              The Book will be inserted to the back of the Catalog list in-order to check if the submission is a duplicate. 
 *              Duplicate Book submissions will be ignored, and a DUPLICATE response message will be returned. Otherwise, a SUBMITTED response message is returned.
 * 
 * Parameter: head                  A pointer to the Book Catalog's head
 * Parameter: title                 The title of the Book to submit
 * Parameter: author                The name of the author of the Book to submit
 * Parameter: location              The location of the Book to submit
 * Parameter: childfd               The socket connection to the client
 * Return: None
*/ 
void submitBook(Book** head, char title[100], char author[100], char location[100], int childfd);



//Main loop
int main(int argc, char **argv) {
    
    //The child socket number
    int childfd; 

    //The port number to listen on
    int portNum;

    //The length of the client's address
    int clientLength;

    //The server address struct
    struct sockaddr_in serveraddr; 

    //The client address struct
    struct sockaddr_in clientaddr; 

    //The hostnet information for the client
    struct hostent *hostp; 

    //The host IP address string
    char *hostaddrp;

    //Flag value for setsockopt
    int optval;

    //Bind the CTRL+C shortcut to an event handler
    sigset(SIGINT, &handleServerClose);
    sigset(SIGTERM, &handleServerClose);

    //Verify the user provided a port number to connect to
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    //Get the port number from the command line
    portNum = atoi(argv[1]);

    //If the user gave a negative port number, exit
    if (portNum < 0) {
        fprintf(stderr, "usage: %s <port> must be non-negative.", argv[1]);
        exit(1);
    }

    //Create the parent (server) socket
    parentfd = socket(AF_INET, SOCK_STREAM, 0);

    //If there was an error creating the parent socket, inform the user
    if (parentfd < 0) {
        perror("ERROR: ");
        exit(1);
    }

    //Code that allows socket to be rebound to immediately without error
    optval = 1;
    setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    //Build the server's internet address with an IP address and port number
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portNum);

    //Bind the parent socket id to the port number
    if (bind(parentfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
        perror("ERROR: ");
        exit(1);
    }

    //Listen for any connection requests, up to 5
    if (listen(parentfd, 5) < 0) {
        perror("ERROR: ");
        exit(1);
    }

    //Get the length of the client's address
    clientLength = sizeof(clientaddr);
    
    //Main loop to wait for a connection request
    while (1) {

        //Wait for a client to connect
        childfd = accept(parentfd, (struct sockaddr *) &clientaddr, (socklen_t*) &clientLength);
        
        //If the client's connection wasn't accepted, error
        if (childfd < 0) {
            perror("ERROR: ");
            exit(1);
        }
        
        //Determine who the client is by their IP address
        hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        
        //If something went wrong getting the host address
        if (hostp == NULL) {
            perror("ERROR: ");
            exit(1);
        }

        //Get the client's IP address
        hostaddrp = inet_ntoa(clientaddr.sin_addr);
        
        //If there was an error getting the client's IP address inform the user
        if (hostaddrp == NULL) {
            perror("ERROR: ");
            exit(1);
        }

        //If the connection was established successfully, inform the user
        printf("Server established connection with %s (%s), and socket fd %d.\n", hostp->h_name, hostaddrp, childfd);

        //Create a thread for the client's connection
        pthread_t clientThread;

        //Bind the main loop function to the thread and pass the child fd
        pthread_create(&clientThread, NULL, launchClientLoop, (void*) &childfd);

        //Detatch the thread so that it will close automatically
        pthread_detach(clientThread);

    }

    return 0;
}



//Event signal handler to avoid port number not closing properly
void handleServerClose() {
    
    // global server socket parentfd
    close(parentfd);

    //Clear the online catalog
    removeAllBooks(&bookCatalog);
    
    //Close the server
    exit(0);
}



//FUNCTION sendServerResponse
void sendServerResponse(int childfd, char response[100], int length) {

    //Inform the user that their request was invalid
    int sentBytes = write(childfd, response, length);

    //If the response wasn't sent, inform the user
    if (sentBytes < 0) {
        fprintf(stderr, "ERROR: The server response was not sent.\n");
        perror("ERROR: ");
        exit(1);
    }
}



//FUNCTION launchClientLoop
void* launchClientLoop(void* fd) {

    //Get the client's socket ID (Responses are only returned to this client)
    int childfd = *((int*)fd);

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
            fprintf(stderr, "ERROR: Client request could not be read.\n");
            perror("ERROR: ");
            exit(1);
        }

        //Else if the client disconnects, stop listening for requests
        else if (requestLength == 0) {
            printf("Client with socket fd %d has disconnected.\n", childfd);
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
                sendServerResponse(childfd, "404:BAD REQUEST,MESSAGE:Request Message is missing ending newline character.\n", 78);
            }
        }
    }

    return NULL;
}



//FUNCTION decipherRequest 
void decipherRequest(char request[], int childfd) {

     //Have the thread check if it can send a request message, otherwise have it wait
    sem_wait(&mutex);

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

        //SUBMIT A BOOK
        submitBook(&bookCatalog, requestTitle, requestAuthor, requestLocation, childfd);
    }

    //GET REQUEST
    else if (strcmp(requestHeaderValue, "GET") == 0) {

        //Parse the request for the first METHOD field and value
        parseRequest(request, requestMethodType, requestMethodValue);

        //If the METHOD field is "AUTHOR", this is a GET Request for books by an AUTHOR
        if (strcmp(requestMethodType, "AUTHOR") == 0) {

            //Copy the Book's author
            strcpy(requestAuthor, requestMethodValue);

            //GET BOOKS BY AUTHOR
            getBooksByAuthor(bookCatalog, requestAuthor, childfd);
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
                getSpecificBook(bookCatalog, requestTitle, requestAuthor, childfd);
            }

            //Else the request doesn't have an AUTHOR field
            else {

                //GET BOOKS WITH TITLE
                getBooksWithTitle(bookCatalog, requestTitle, childfd);
            }
        }

        //Else the METHOD field is invalid
        else {
            
            //Inform the user that their request was invalid
            sendServerResponse(childfd, "404:BAD REQUEST\nMESSAGE:Request Message has an invalid field.", 62);
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
        removeBook(&bookCatalog, requestTitle, requestAuthor, requestLocation, childfd);
    }

    //INVALID REQUEST (Needs to be turned into a WRITE ERROR EVENTUALLY)
    else {
        sendServerResponse(childfd, "404:BAD REQUEST\nMESSAGE:Request Message is an invalid type.\n", 61);
    }

    //Once a thread has had its request read and a response returned, unblock other threads
    sem_post(&mutex);
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



//Search the list by book title
void getBooksByAuthor(Book* head, char author[100], int childfd) {

    //Boolean to check if any Book matches were found
    bool found = false;

    //The response message to send back to the client
    char* serverResponse;
    serverResponse = malloc(sizeof(char) * 1000);

    //Iterate though
    while(head != NULL) {

        //if a match is found add it to the matchList string
        if (strcmp(head->author, author) == 0) {

            //If this is the first match, set up the server response
            if (found == false) {
                found = true;
                strcpy(serverResponse, "202:RETRIEVED\n");
            }

            //Append the matched Book Location to the Server Response
            strcat(serverResponse, "TITLE:");
            strcat(serverResponse, head->title);
            strcat(serverResponse, "\n");
            strcat(serverResponse, "AUTHOR:");
            strcat(serverResponse, author);
            strcat(serverResponse, "\n");
            strcat(serverResponse, "LOCATION:");
            strcat(serverResponse, head->location);
            strcat(serverResponse, "\n\n");
        }

        head = head->next;
    }

    //If there were Books found for the given title, inform the user
    if (found == false) {
        strcpy(serverResponse, "402:NOT FOUND\nMESSAGE:There are no Books in the Catalog with the given author.\n");
        sendServerResponse(childfd, serverResponse, strlen(serverResponse));
    }

    //If there were Books found for the given title    
    else {

        //Pass the Book locations to the user
        sendServerResponse(childfd, serverResponse, strlen(serverResponse));
    }

    //Free the server response message
    free(serverResponse);
}



//Search the list by book title
void getBooksWithTitle(Book* head, char title[100], int childfd) {

    //Boolean to check if any Book matches were found
    bool found = false;

    //The response message to send back to the client
    char* serverResponse;
    serverResponse = malloc(sizeof(char) * 1000);

    //Iterate though
    while(head != NULL) {

        //if a match is found add it to the matchList string
        if (strcmp(head->title, title) == 0) {

            //If this is the first match, set up the server response
            if (found == false) {
                found = true;
                strcpy(serverResponse, "202:RETRIEVED\n");
            }

            //Append the matched Book Location to the Server Response
            strcat(serverResponse, "TITLE:");
            strcat(serverResponse, title);
            strcat(serverResponse, "\n");
            strcat(serverResponse, "AUTHOR:");
            strcat(serverResponse, head->author);
            strcat(serverResponse, "\n");
            strcat(serverResponse, "LOCATION:");
            strcat(serverResponse, head->location);
            strcat(serverResponse, "\n\n");
        }

        head = head->next;
    }

    //If there were Books found for the given title, inform the user
    if (found == false) {
        strcpy(serverResponse, "402:NOT FOUND\nMESSAGE:There are no Books in the Catalog with the given title.\n");
        sendServerResponse(childfd, serverResponse, strlen(serverResponse));
    }

    //If there were Books found for the given title
    else {

        //Pass the Book locations to the user
        sendServerResponse(childfd, serverResponse, strlen(serverResponse));
    }

    //Free the server response message
    free(serverResponse);
}



//Search the list for the Specified Book
void getSpecificBook(Book* head, char title[100], char author[100], int childfd) {

    //Boolean to check if any Book matches were found
    bool found = false;

    //The response message to send back to the client
    char* serverResponse;
    serverResponse = malloc(sizeof(char) * 1000);

    //Iterate though
    while(head != NULL) {

        //if a match is found add it to the matchList string
        if (strcmp(head->title, title) == 0 && strcmp(head->author, author) == 0) {

            //If this is the first match, set found to true and build the response message 
            if (found == false) {
                found = true;
                strcpy(serverResponse, "202:RETRIEVED\n");
            }

            //Append the matched Book Location to the Server Response
            strcat(serverResponse, "LOCATION:");
            strcat(serverResponse, head->location);
            strcat(serverResponse, "\n\n");
        }

        //Check the next Book in the Catalog
        head = head->next;
    }

    //If there were Books found for the given title, inform the user
    if (found == false) {
        strcpy(serverResponse, "402:NOT FOUND\nMESSAGE: There were no Books with the given title and author in the Catalog.\n");
        sendServerResponse(childfd, serverResponse, strlen(serverResponse));
    }

    //If there were Books found for the given title
    else {
        
        //Pass the Book locations to the user
        sendServerResponse(childfd, serverResponse, strlen(serverResponse));
    }

    //Free the server response message
    free(serverResponse);
}



//FUNCTION Remove ALL Books from Catalog
void removeAllBooks(Book** head) {

    //Temporary Book object for modifying the Catalog
    Book* temp = *head;

    //If the Catalog is empty, return
    if (temp == NULL) {
        return;
    }

    //Otherwise,
    else {

        //Free the first n-1 Books in the Catalog
        while (temp->next != NULL) {
            temp = temp->next;
            free(temp->previous);
        }

        //Free the last Book in the Catalog and set the head pointer to NULL
        free(temp);
        temp = NULL;

        //Save the changes made to the original Catalog
        *head = temp;
    }
}



//FUNCTION Remove a specific book from the list
void removeBook(Book** head, char title[100], char author[100], char location[100], int childfd) {

    Book* temp = *head;

    //The response message to send back to the client
    char* serverResponse;
    serverResponse = malloc(sizeof(char) * 1000);

    //CASE ONE: BOOK CATALOG IS EMPTY
    if (temp == NULL) {
        
        strcpy(serverResponse, "402:NOT FOUND\nMESSAGE:The Book Catalog is empty and thus does not contain the Book specified.\n");
        sendServerResponse(childfd, serverResponse, strlen(serverResponse));
    }

    //CASE TWO: BOOK CATALOG HAS ONE BOOK
    else if (temp->next == NULL) {

        //If the Book being searched for is the sole one
        if (strcmp(temp->author, author) == 0 && strcmp(temp->title, title) == 0 && strcmp(temp->location, location) == 0) {

            //Free the book
            free(temp);
            temp = NULL;

            //Set the passed head pointer to the temporary one to save the changes
            *head = temp;

            //Build the REMOVED Server Response Message
            strcpy(serverResponse, "203:REMOVED\nTITLE:");
            strcat(serverResponse, title);
            strcat(serverResponse, "\nAUTHOR:");
            strcat(serverResponse, author);
            strcat(serverResponse, "\nLOCATION:");
            strcat(serverResponse, location);
            strcat(serverResponse, "\n");

            //Send the Server Response Message
            sendServerResponse(childfd, serverResponse, strlen(serverResponse));   
        } 

        //If the Book being searched for is not the sole one
        else {
            
            //Write to the client that the book was not found
            strcpy(serverResponse, "402:NOT FOUND\nMESSAGE:The Book specified could not be found in the Catalog.\n");
            sendServerResponse(childfd, serverResponse, strlen(serverResponse));
        }
    }

    //CASE THREE: BOOK CATALOG HAS MORE THAN ONE BOOK
    else {

        //Temporary Book iterator for the Catalog
        Book* it = temp;

        //Iterate through the Book Catalog for the desired Book to Remove
        while(it != NULL) {

            //If there's a match, stop iterating
            if (strcmp(it->title, title) == 0 && strcmp(it->author, author) == 0 && strcmp(it->location, location) == 0) {
                break;
            }

            //Keep iterating
            else {
                it = it->next;
            }
        }

        //If the Book wasn't found in the list
        if (it == NULL) {

            //Write to the client that the book was not found
            strcpy(serverResponse, "402:NOT FOUND\nMESSAGE:The Book specified could not be found in the Catalog.\n");
            sendServerResponse(childfd, serverResponse, strlen(serverResponse));
        }

        //Otherwise, remove the Book entry based on its position in the Catalog
        else {

            //If this is the first Book in the Catalog
            if (it->previous == NULL && it->next != NULL) {

                //Set the second last Book's previous pointer to NULL
                it->next->previous = NULL;

                //Move the head pointer to the second Book
                temp = it->next;

                //Free the Book
                free(it);        

                //Set the passed head pointer to the temporary one to save the changes
                *head = temp;

                //Build the REMOVED Server Response Message
                strcpy(serverResponse, "203:REMOVED\nTITLE:");
                strcat(serverResponse, title);
                strcat(serverResponse, "\nAUTHOR:");
                strcat(serverResponse, author);
                strcat(serverResponse, "\nLOCATION:");
                strcat(serverResponse, location);
                strcat(serverResponse, "\n");

                //Send the Server Response Message
                sendServerResponse(childfd, serverResponse, strlen(serverResponse));   
            }

            //If this is the last Book in the Catalog
            if (it->previous != NULL && it->next == NULL) {

                //Set the second last Book's next pointer to NULL
                it->previous->next = NULL;

                //Free the Book node
                free(it);

                //Set the passed head pointer to the temporary one to save the changes
                *head = temp;

                //Build the REMOVED Server Response Message
                strcpy(serverResponse, "203:REMOVED\nTITLE:");
                strcat(serverResponse, title);
                strcat(serverResponse, "\nAUTHOR:");
                strcat(serverResponse, author);
                strcat(serverResponse, "\nLOCATION:");
                strcat(serverResponse, location);
                strcat(serverResponse, "\n");

                //Send the Server Response Message
                sendServerResponse(childfd, serverResponse, strlen(serverResponse));   
            }

            //This Book lies in the middle of the Catalog
            else {

                //Set the previous node to point to the one after the one being deleted
                it->previous->next = it->next;

                //Set the next node to point to the one before the one being deleted
                it->next->previous = it->previous;

                //Free the node
                free(it);

                //Set the passed head pointer to the temporary one to save the changes
                *head = temp;

                //Build the REMOVED Server Response Message
                strcpy(serverResponse, "203:REMOVED\nTITLE:");
                strcat(serverResponse, title);
                strcat(serverResponse, "\nAUTHOR:");
                strcat(serverResponse, author);
                strcat(serverResponse, "\nLOCATION:");
                strcat(serverResponse, location);
                strcat(serverResponse, "\n");

                //Send the Server Response Message
                sendServerResponse(childfd, serverResponse, strlen(serverResponse));   
            }
        }
    }

    //Free the server response message
    free(serverResponse);
}



//Append a new member to the end of the list -- CHECKED
void submitBook(Book** head, char title[100], char author[100], char location[100], int childfd) {

    //Temporary head pointer to modify the Catalog list
    Book* temp = *head;

    //The new Book to be added
    Book* newBook;

    //The response message to send back to the client
    char* serverResponse;
    serverResponse = malloc(sizeof(char) * 1000);

    //Malloc the new Book Struct
    newBook = malloc(sizeof(Book));

    //add the new data to the entry
    strcpy(newBook->title, title);
    strcpy(newBook->author, author);
    strcpy(newBook->location, location);

    //Since end of list, next will always be NULL
    newBook->next = NULL;
    
    //If list empty, new node is the head
    if(temp == NULL) {
        newBook->previous = NULL;
        newBook->next = NULL;
        temp = newBook;

        //Set the passed head pointer to the temporary one to save the changes
        *head = temp;

        //Form the server response message
        strcpy(serverResponse, "201: SUBMITTED\n");
        strcat(serverResponse, "TITLE:");
        strcat(serverResponse, title);
        strcat(serverResponse, "\n");
        strcat(serverResponse, "AUTHOR:");
        strcat(serverResponse, author);
        strcat(serverResponse, "\n");
        strcat(serverResponse, "LOCATION:");
        strcat(serverResponse, location);
        strcat(serverResponse, "\n");

        //Send the server response message
        sendServerResponse(childfd, serverResponse, strlen(serverResponse));
    }

    else {

        //Otherwise traverse to the end of the list
        Book* it = temp;
        
        bool duplicate = false;

        //Traverse the list to the end
        while(it->next != NULL) {

            //If the Book submission is a duplicate, it can't be added to the Catalog
            if (strcmp(title, it->title) == 0 && strcmp(author, it->author) == 0 && strcmp(location, it->location) == 0) {
                duplicate = true;
                break;
            }

            it = it->next;
        }

        //If this is a duplicate Book submission, inform the user
        if (duplicate == true) {
            
            //Write to the client that the Book is a duplicate
            strcpy(serverResponse, "401:DUPLICATE\nMESSAGE:The Book specified is a duplicate submission and could not be added to the Catalog.\n");
            sendServerResponse(childfd, serverResponse, strlen(serverResponse));
        }

        //Insert the next and previous node info
        newBook->previous = it;
        it->next = newBook;

        //Set the passed head pointer to the temporary one to save the changes
        *head = temp;

        //Form the server response message
        strcpy(serverResponse, "201: SUBMITTED\n");
        strcat(serverResponse, "TITLE:");
        strcat(serverResponse, title);
        strcat(serverResponse, "\n");
        strcat(serverResponse, "AUTHOR:");
        strcat(serverResponse, author);
        strcat(serverResponse, "\n");
        strcat(serverResponse, "LOCATION:");
        strcat(serverResponse, location);
        strcat(serverResponse, "\n");

        //Send the server response message
        sendServerResponse(childfd, serverResponse, strlen(serverResponse));
    }

    //Free the server response message
    free(serverResponse);
}
