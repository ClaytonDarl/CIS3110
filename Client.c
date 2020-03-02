#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>



/* Name: collectBookInformation
 * Description: This function collects information for a Book from the user in-order to
 *              SUBMIT, GET, or REMOVE Book(s) to/from the Book Catalog.
 * 
 * Parameter: menuChoice            The user's menu choice used to determine what request they selected
 * Parameter: sockfd                The socket connection to the server
 * Return: None
 */ 
void collectBookInformation(int menuChoice, int sockfd);



/* Name: getBooksByAuthor
 * Description: This function will attempt to get the Books from the server's Book Catalog for the given author.
 * 
 * Parameter: bookAuthor        The author of the book
 * Parameter: sockfd            The socket connection to the server
 * Return: None
*/ 
void getBooksByAuthor(char bookAuthor[], int sockfd);



/* Name: getBooksWithTitle
 * Description: This function will attempt to get the Books from the server's Book Catalog with the given title.
 * 
 * Parameter: bookTitle         The title of the book
 * Parameter: sockfd            The socket connection to the server
 */ 
void getBooksWithTitle(char bookTitle[], int sockfd);



/* Name: getSpecificBook
 * Description: This function will attempt to get the Book from the server's Book Catalog with the passed information.
 *
 * Parameter: bookTitle         The title of the book
 * Parameter: bookAuthor        The author of the book
 * Parameter: sockfd            The socket connection to the server
 * Return: None
*/  
void getSpecificBook(char bookTitle[], char bookAuthor[], int sockfd);



/* Name: removeBook
 * Description: This function will attempt to remove the Book from the server's Book Catalog with the passed information.
 * 
 * Parameter: bookTitle         The title of the Book
 * Parameter: bookAuthor        The author of the Book
 * Parameter: bookLocation      The location of the Book
 * Parameter: sockfd            The socket connection to the server
 * Return: None
*/  
void removeBook(char bookTitle[], char bookAuthor[], char bookLocation[], int sockfd);



/* Name: submitBook
 * Description: This function will attempt to submit a Book to the server's Book Catalog with the passed information.
 * 
 * Parameter: bookTitle         The title of the Book
 * Parameter: bookAuthor        The author of the Book
 * Parameter: bookLocation      The location of the Book
 * Parameter: sockfd            The socket connection to the server 
 * Return: None
*/ 
void submitBook(char bookTitle[], char bookAuthor[], char bookLocation[], int sockfd);



//Main loop
int main(int argc, char **argv) {

    int sockfd;
    int portNum;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char hostName[20];

    //The user's menu choice
    char menuChoice = '0';

    //Verify the user specified a host and port number
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(1);
    }

    //Grab the hostname and port number provided by the user
    strcpy(hostName, argv[1]);
    portNum = atoi(argv[2]);

    //If the user gave a negative port number, exit
    if (portNum < 0) {
        fprintf(stderr, "usage: %s <port> must be non-negative.", argv[2]);
        exit(1);
    }

    //Create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    //If the socket couldn't be created, inform the user
    if (sockfd < 0) {
        perror("ERROR: ");
        exit(1);
    }

    //Get the server's DNS entry
    server = gethostbyname(hostName);

    //If the hostname provided was invalid, inform the user
    if (server == NULL) {
        fprintf(stderr,"usage: Hostname provides doesn't exist. %s\n", hostName);
        exit(1);
    }

    //Build the internet address
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char*)server->h_addr_list[0], (char*)&serveraddr.sin_addr.s_addr, server->h_length);

    //Convert the port number to network byte order
    serveraddr.sin_port = htons(portNum);

    //Create a connection with the server
    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("ERROR: ");
        exit(1);
    }

    //Continue to run the program until the user decides to quit
    while (menuChoice != '6') {
        printf("Please select one of the below menu options.\n");
        printf("1: SUBMIT a Book to the Book Catalog.\n");
        printf("2: GET locations of a specific Book from the Book Catalog.\n");
        printf("3: GET all Books by an author from the Book Catalog.\n");
        printf("4: GET all Books with a given title from the Book Catalog.\n");
        printf("5: REMOVE a Book from the Book Catalog.\n");
        printf("6: EXIT the program.\n\n");

        //Prompt for the user's menu selection
        printf(">: ");
        scanf("%c", &menuChoice);

        //Clear the input buffer of any leftover characters
        while ((getchar()) != '\n'); 

        //MENU CHOICE 1: SUBMIT a Book
        if (menuChoice == '1') {
            collectBookInformation(1, sockfd);
        }

        //MENU CHOICE 2: GET a Book
        else if (menuChoice == '2') {
            collectBookInformation(2, sockfd);
        }

        //MENU CHOICE 3: GET all Books by the given author
        else if (menuChoice == '3') {
            collectBookInformation(3, sockfd);
        }

        //MENU CHOICE 4: GET all Books with the given title
        else if (menuChoice == '4') {
            collectBookInformation(4, sockfd);
        }

        //MENU CHOICE 5: REMOVE a Book from the Book Catalog
        else if (menuChoice == '5') {
            collectBookInformation(5, sockfd);
        }

        //MENU CHOICE 6: EXIT
        else if (menuChoice == '6') {
            printf("Goodbye!\n");
        }

        //If the user gives bad input, let them know
        else {
            fprintf(stderr, "usage: Invalid menu option. Please try again.\n\n");
        }
    }

    //Close the client socket connection
    close(sockfd);

    return 0;
}



//FUNCTION collectBookInformation
void collectBookInformation(int menuChoice, int sockfd) {

    //The Input Buffer for collecting data
    char inputBuffer[1000];

    //The Book's fields collected from the user
    char bookTitle[100];
    char bookAuthor[100];
    char bookLocation[100];

    //The length of the Book's fields collected from the user
    int titleLength;
    int authorLength;
    int locationLength;

    //OPTION ONE: SUBMIT A BOOK
    if (menuChoice == 1) {

        //Prompt the user for the Book's title
        printf("Please enter the name of the new Book to submit. Max 100 Characters.\n");

        //Collect the input buffer, and copy the first 100 characters into the Book Title
        fgets(inputBuffer, 1000, stdin);
        strncpy(bookTitle, inputBuffer, 100);

        //Get the length of the submitted Book Title
        titleLength = strlen(inputBuffer) - 1;

        printf("\n");

        //Prompt the user for the Book's author
        printf("Please enter the author of the new Book to submit. Max 100 Characters.\n");

        //Collect the input buffer, and copy the first 100 characters into the Book Author
        fgets(inputBuffer, 1000, stdin);
        strncpy(bookAuthor, inputBuffer, 100);

        //Get the length of the submitted Book Title
        authorLength = strlen(inputBuffer) - 1;

        printf("\n");

        //Prompt the user for the Book's location and get its length
        printf("Please enter the location of the new Book to submit. Max 100 Characters.\n");

        //Collect the input buffer, and copy the first 100 characters into the Book Location
        fgets(inputBuffer, 1000, stdin);
        strncpy(bookLocation, inputBuffer, 100);

        //Get the length of the submitted Book Title
        locationLength = strlen(inputBuffer) - 1;

        printf("\n");

        //If any entered fields were blank, inform the user
        if (titleLength == 0 || authorLength == 0 || locationLength == 0) {
            printf("usage: One of the entered Book information entries was blank. Please try again.\n\n");
        }

        //If any entered fields were too long, inform the user
        else if (titleLength > 100 || authorLength > 100 || locationLength > 100) {
            printf("usage: One of the entered Book information entries was too long. Max 100 Characters. Please try again.\n\n");
        }

        //Else
        else {

            //Remove trailing newlines from the input
            bookTitle[titleLength] = '\0';
            bookAuthor[authorLength] = '\0';
            bookLocation[locationLength] = '\0';

            //Call the submitBook function with the inputs
            submitBook(bookTitle, bookAuthor, bookLocation, sockfd);
        }
    }

    //OPTION TWO: GET A SPECIFIC BOOK
    else if (menuChoice == 2) {

        //Prompt the user for the Book's title and get its length
        printf("Please enter the name of the Book to retrieve. Max 100 Characters.\n");

        //Collect the input buffer, and copy the first 100 characters into the Book Title
        fgets(inputBuffer, 1000, stdin);
        strncpy(bookTitle, inputBuffer, 100);

        //Get the length of the submitted Book Title
        titleLength = strlen(inputBuffer) - 1;

        printf("\n");

        //Prompt the user for the Book's author and get its length
        printf("Please enter the author of the Book to retrieve. Max 100 Characters.\n");

        //Collect the input buffer, and copy the first 100 characters into the Book Author
        fgets(inputBuffer, 1000, stdin);
        strncpy(bookAuthor, inputBuffer, 100);

        //Get the length of the submitted Book Author
        authorLength = strlen(inputBuffer) - 1;

        printf("\n");

        //Verify that the entered fields aren't blank
        if (titleLength == 0 || authorLength == 0) {
            fprintf(stderr, "usage: One of the entered Book information entries was blank. Please try again.\n\n");
        }

        //If any entered fields were too long, inform the user
        else if (titleLength > 100 || authorLength > 100) {
            printf("usage: One of the entered Book information entries was too long. Max 100 Characters. Please try again.\n\n");
        }

        //Else
        else {
            //Remove trailing newlines from the input
            bookTitle[titleLength] = '\0';
            bookAuthor[authorLength] = '\0';

            //Call the getSpecificBook function
            getSpecificBook(bookTitle, bookAuthor, sockfd);
        }
    }

    //OPTION THREE: GET BOOKS BY AUTHOR
    else if (menuChoice == 3) {
        
        //Prompt the user for the Book's author and get its length
        printf("Please enter the author of the Books to retrieve. Max 100 Characters.\n");
 
        //Collect the input buffer, and copy the first 100 characters into the Book Author
        fgets(inputBuffer, 1000, stdin);
        strncpy(bookAuthor, inputBuffer, 100);

        //Get the length of the submitted Book Author
        authorLength = strlen(inputBuffer) - 1;

        printf("\n");

        //Verify that the entered field isn't blank
        if (authorLength == 0) {
            fprintf(stderr, "usage: The Book author entered can't be blank. Please try again.\n\n");
        }

        //Verify that the entered field isn't too long
        else if (authorLength > 100) {
            fprintf(stderr, "usage: The Book author entered is too long. Max 100 Characters. Please try again.\n\n");
        }

        //Else
        else {

            //Remove trailing newlines from the input
            bookAuthor[authorLength] = '\0';

            //Call the getBooksByAuthor Function
            getBooksByAuthor(bookAuthor, sockfd);
        }
    }

    //OPTION FOUR: GET BOOKS WITH A GIVEN TITLE
    else if (menuChoice == 4) {

        //Prompt the user for the Book's title and get its length
        printf("Please enter the title of the Books to retrieve. Max 100 characters. \n");
        
        //Collect the input buffer, and copy the first 100 characters into the Book Title
        fgets(inputBuffer, 1000, stdin);
        strncpy(bookTitle, inputBuffer, 100);

        //Get the length of the submitted Book Title
        titleLength = strlen(inputBuffer) - 1;

        printf("\n");

        //Verify that the entered fields aren't blank
        if (titleLength == 0) {
            fprintf(stderr, "usage: The Book title entered can't be blank. Please try again.\n\n");
        }

        //Verify that the entered field isn't too long
        else if (titleLength > 100) {
            fprintf(stderr, "usage: The Book title entered is too long. Max 100 Characters. Please try again.\n\n");
        }

        //Else
        else {

            //Remove trailing newlines from the input
            bookTitle[titleLength] = '\0';

            //Call the getBooksWithTitle Function
            getBooksWithTitle(bookTitle, sockfd);
        }
    }

    //OPTION FIVE: REMOVE A BOOK 
    else if (menuChoice == 5) {

        //Prompt the user for the Book's title and get its length
        printf("Please enter the title of the Book to remove. Max 100 characters.\n");
        fgets(bookTitle, 100, stdin);
        titleLength = strlen(bookTitle) - 1;

        printf("\n");

        //Prompt the user for the Book's author and get its length
        printf("Please enter the author of the Book to remove. Max 100 characters.\n");
        fgets(bookAuthor, 100, stdin);
        authorLength = strlen(bookAuthor) - 1;

        printf("\n");

        //Prompt the user for the Book's location and get its length
        printf("Please enter the location of the Book to remove. Max 100 characters.\n");
        fgets(bookLocation, 100, stdin);
        locationLength = strlen(bookLocation) - 1;

        printf("\n");

        //If any entered fields were blank, inform the user
        if (titleLength == 0 || authorLength == 0 || locationLength == 0) {
            fprintf(stderr, "usage: One of the entered Book information entries was blank. Please try again.\n\n");
        }

        //If any entered fields were too long, inform the user
        else if (titleLength > 100 || authorLength > 100 || locationLength > 100) {
            printf("usage: One of the entered Book information entries was too long. Max 100 Characters. Please try again.\n\n");
        }

        //Else
        else {
                        
            //Remove trailing newlines from the input
            bookTitle[titleLength] = '\0';
            bookAuthor[authorLength] = '\0';
            bookLocation[locationLength] = '\0';

            //Call the removeBook function with the inputs
            removeBook(bookTitle, bookAuthor, bookLocation, sockfd);
        }
    }

    //INVALID MENU OPTIONS ARE IGNORED
    else {
        fprintf(stderr, "usage: Invalid menu option. Options are 1-5 for contacting the server and 6 to exit. Please try again.\n");
    }
}


//FUNCTION getBooksByAuthor
void getBooksByAuthor(char bookAuthor[], int sockfd) {

    //The Book get request to send to the server
    char getRequest[250];

    //The response from the server
    char *serverResponse;

    //The length of the SUBMIT request sent to the server
    int requestLength = 0;

    //The length of the response received from the server
    int responseLength = 0;

    //Form the Book REMOVE request
    strcpy(getRequest, "METHOD:GET,");
    strcat(getRequest, "AUTHOR:");
    strcat(getRequest, bookAuthor);
    strcat(getRequest, "\n");

    //Send the GET request to the server
    requestLength = write(sockfd, getRequest, strlen(getRequest));
    
    //If the request wasn't sent to the server, inform the user
    if (requestLength <= 0) {
        fprintf(stderr, "ERROR: Client request was not sent to server.\n");
        perror("ERROR: ");
        exit(1);
    }

    //Malloc room for the server's response
    serverResponse = malloc(sizeof(char)*1000);

    //Get the server's response message
    responseLength = read(sockfd, serverResponse, 1000);
    
    //If the response message couldn't be read, inform the user
    if (responseLength <= 0) {
        fprintf(stderr, "ERROR: Server response was not received.\n");
        perror("ERROR: ");
        exit(1);
    }


    //Otherwise, display the response message
    else {
        
        //Remove trailing newlines from the input
        serverResponse[responseLength] = '\0';
        printf("Server response:\n%s\n", serverResponse);
    }

    free(serverResponse);
}



//FUNCTION getBooksWithTitle
void getBooksWithTitle(char bookTitle[], int sockfd) {

   //The Book GET request to send to the server
    char getRequest[250];

    //The response from the server
    char *serverResponse;

    //The length of the SUBMIT request sent to the server
    int requestLength = 0;

    //The length of the response received from the server
    int responseLength = 0;

    //Form the Book GET request
    strcpy(getRequest, "METHOD:GET,");
    strcat(getRequest, "TITLE:");
    strcat(getRequest, bookTitle);
    strcat(getRequest, "\n");

    //Malloc room for the server's response
    serverResponse = malloc(sizeof(char)*1000);

    //Send the GET request to the server
    requestLength = write(sockfd, getRequest, strlen(getRequest));
    
    //If the request wasn't sent to the server, inform the user
    if (requestLength <= 0) {
        fprintf(stderr, "ERROR: Client request was not sent to server.\n");
        perror("ERROR: ");
        exit(1);
    }

    //Get the server's response message
    responseLength = read(sockfd, serverResponse, 1000);
    
    //If the response message couldn't be read, inform the user
    if (responseLength <= 0) {
        fprintf(stderr, "ERROR: Server response was not received.\n");
        perror("ERROR: ");
        exit(1);
    }

    //Otherwise, display the response message
    else {

        //Remove trailing newlines from the input
        serverResponse[responseLength] = '\0';
        printf("Server response:\n%s\n", serverResponse);
    }

    //Free the server response
    free(serverResponse);
}



//FUNCTION getSpecificBook
void getSpecificBook(char bookTitle[], char bookAuthor[], int sockfd) {

    //The Book GGT request to send to the server
    char getRequest[250];

    //The response from the server
    char *serverResponse;

    //The length of the GET request sent to the server
    int requestLength = 0;

    //The length of the response received from the server
    int responseLength = 0;

    //Form the Book REMOVE request
    strcpy(getRequest, "METHOD:GET,");
    strcat(getRequest, "TITLE:");
    strcat(getRequest, bookTitle);
    strcat(getRequest, ",");
    strcat(getRequest, "AUTHOR:");
    strcat(getRequest, bookAuthor);
    strcat(getRequest, "\n");

    //Send the GET request to the server
    requestLength = write(sockfd, getRequest, strlen(getRequest));
    
    //If the request wasn't sent to the server, inform the user
    if (requestLength <= 0) {
        fprintf(stderr, "ERROR: Client request was not sent to server.\n");
        perror("ERROR: ");
        exit(1);
    }

    //Malloc room for the server's response
    serverResponse = malloc(sizeof(char)*1000);

    //Get the server's response message
    responseLength = read(sockfd, serverResponse, 1000);
    
    //If the response message couldn't be read, inform the user
    if (responseLength < 0) {
        fprintf(stderr, "ERROR: Server response was not received.\n");
        perror("ERROR: ");
        exit(1);
    }

    //Otherwise, display the response message
    else {

        //Remove trailing newlines from the input
        serverResponse[responseLength] = '\0';

        printf("Server response:\n%s\n", serverResponse);
    }

    //Free the server response
    free(serverResponse);
}



//Function: removeBook()
void removeBook(char bookTitle[], char bookAuthor[], char bookLocation[], int sockfd) {
    
    //The Book REMOVE request to send to the server
    char removeRequest[250];

    //The response from the server
    char *serverResponse;

    //The length of the SUBMIT request sent to the server
    int requestLength = 0;

    //The length of the response received from the server
    int responseLength = 0;

    //Form the Book REMOVE request
    strcpy(removeRequest, "METHOD:REMOVE,");
    strcat(removeRequest, "TITLE:");
    strcat(removeRequest, bookTitle);
    strcat(removeRequest, ",");
    strcat(removeRequest, "AUTHOR:");
    strcat(removeRequest, bookAuthor);
    strcat(removeRequest, ",");
    strcat(removeRequest, "LOCATION:");
    strcat(removeRequest, bookLocation);
    strcat(removeRequest,"\n");

    //Malloc room for the server's response
    serverResponse = malloc(sizeof(char)*1000);

    //Send the REMOVE request to the server
    requestLength = write(sockfd, removeRequest, strlen(removeRequest));
    
    //If the request wasn't sent to the server, inform the user
    if (requestLength <= 0) {
        fprintf(stderr, "ERROR: Client request was not sent to server.\n");
        perror("ERROR: ");
        exit(1);
    }

    //Get the server's response message
    responseLength = read(sockfd, serverResponse, 1000);
    
    //If the response message couldn't be read, inform the user
    if (responseLength < 0) {
        fprintf(stderr, "ERROR: Server response was not received.\n");
        perror("ERROR: ");
        exit(1);
    }

    //Otherwise, display the response message
    else {

        //Remove trailing newlines from the input
        serverResponse[responseLength] = '\0';

        printf("Server response:\n%s\n", serverResponse);
    }

    //Free the server response
    free(serverResponse);
}



//Function: submitBook()
void submitBook(char bookTitle[], char bookAuthor[], char bookLocation[], int sockfd) {

    //The Book SUBMIT request to send to the server
    char submitRequest[250];

    //The response from the server
    char *serverResponse;

    //The length of the SUBMIT request sent to the server
    int requestLength = 0;

    //The length of the response received from the server
    int responseLength = 0;

    //Form the Book SUBMIT request
    strcpy(submitRequest, "METHOD:SUBMIT,");
    strcat(submitRequest, "TITLE:");
    strcat(submitRequest, bookTitle);
    strcat(submitRequest, ",");
    strcat(submitRequest, "AUTHOR:");
    strcat(submitRequest, bookAuthor);
    strcat(submitRequest, ",");
    strcat(submitRequest, "LOCATION:");
    strcat(submitRequest, bookLocation);
    strcat(submitRequest,"\n");

    //Send the SUBMIT request to the server
    requestLength = write(sockfd, submitRequest, strlen(submitRequest));

    //If the request wasn't sent to the server, inform the user
    if (requestLength <= 0) {
        fprintf(stderr, "ERROR: Client request was not sent to server.\n");
        perror("ERROR: ");
        exit(1);
    }

    //Malloc room for the server's response
    serverResponse = malloc(sizeof(char)*1000);

    //Get the server's response message
    responseLength = read(sockfd, serverResponse, 1000);
    
    //If the response message couldn't be read, inform the user
    if (responseLength < 0) {
        fprintf(stderr, "ERROR: Server response was not received.\n");
        perror("ERROR: ");
        exit(1);
    }

    //Otherwise, display the response message
    else {

        //Remove trailing newlines from the input
        serverResponse[responseLength] = '\0';
        
        printf("Server response:\n%s\n", serverResponse);
    }

    //Free the server response
    free(serverResponse);
}
