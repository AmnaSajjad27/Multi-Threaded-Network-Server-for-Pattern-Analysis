#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// For locks
#include <pthread.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

// Define ports
#define PORT 12345
#define MAX_BUFFER_SIZE 1024

// Linked List Structure
typedef struct Node
{
    char data[MAX_BUFFER_SIZE];
    struct Node* next;
    struct Node* book_next;
    struct Node* next_frequent_search;
}Node;

// Shared List Structure
typedef struct Shared_list
{
    Node* head;
    Node* tail;
    pthread_mutex_t lock;
}Shared_list;

// Initaliseing a Shared list object
Shared_list sharedlist = {NULL, NULL, PTHREAD_MUTEX_INITIALIZER};

// Search pattern structure
typedef struct search_Pattern
{
    char pattern[MAX_BUFFER_SIZE];
    int frequency;
    pthread_t output_thread;
}search_Pattern;

// Initalise a search pattern 
search_Pattern searchPattern = {"", 0, 0};

// Add node to sharedlist
void add_Node(const char* data)
{
    // Allocate memory for new data
    Node* newNode = (Node*)malloc(sizeof(Node));
    // strncpy(char *dest, const char *src, size_t n)
    strncpy(newNode->data, data, MAX_BUFFER_SIZE);
    newNode->next = NULL;
    newNode->book_next = NULL;
    newNode->next_frequent_search = NULL;
    // Lock a mutex and acquire a lock
    pthread_mutex_lock(&sharedlist.lock);
    // If list is currently empty 
    if (sharedlist.head == NULL)
    {
        // Set the first node as newNode
        sharedlist.head = newNode;
        // Set the tail as its the only node
        sharedlist.tail = newNode;
    }
    // List is not empty
    else
    {
        // Set the next pointer of current Node to newNode
        sharedlist.head->next = newNode;
        // Update the tail pointer to point to newNode, newNode is the last node in the list
        sharedlist.tail = newNode;
    }
    // Release the lock, done editing the list
    pthread_mutex_unlock(&sharedlist.lock);
}

// Function to print a book
void print_Book(Node* book_head)
{
    // pointer current points to the first node in the Linked list
    Node* current = book_head;
    // Iteriate till there are no more nodes in the list
    while(current != NULL)
    {
        // Print the line from the book
        printf("%s", current->data);
        // Update pointer to the next 
        current = current->book_next;
    }
}

// Function to handle client communication
// Called in a seprate thread for each connected client
void* Handler_Client(void* client_socket)
{
    // de-refrence the argument, client_socket is a pointer to an integer
    int client = *((int*)client_socket);
    // The following two store the data read from the client and the number of bytes read from the client
    char buffer[MAX_BUFFER_SIZE];
    int bytes_read;

    // reading client data using recv till there is no more data to read i.e. buffer more than 0
    // recv(client_socket_fd, buffer_to_store_data, max_size_buffer, flags)
    while ((bytes_read = recv(client, buffer, sizeof(buffer), 0)) > 0)
    {
        // This ensures the string is null terminated
        buffer[bytes_read] = '\0';
        // Add data recieved to shared list using function add_Node
        add_Node(buffer);
        // For debugging, print data has been added 
        printf("%s added\n", buffer);
    }
    // Terminate the connection
    close(client);

    // write book to file, lock the mutex
    pthread_mutex_lock(&sharedlist.lock);
    // Assumming head is at the start of the book
    Node* book_head = sharedlist.head;
    sharedlist.head = NULL;
    // release the lock on mutex
    pthread_mutex_unlock(&sharedlist.lock);

    // Number of books processed 
    static int book_counter = 0;
    char filename[20];
    // print and increment book counter
    book_counter++;
    sprintf(filename, "Book_%02d.txt", book_counter);

    // open file for writting 
    FILE* file = fopen(filename, "w");
    if (file != NULL)
    {
        print_Book(book_head);
        fclose(file);
    }
    // Free Memory allocated for client socket file descriptor 
    free(client_socket);
    return NULL;
}

// Function to analyse pattern
void* analyse_Thread(void* arg)
{
    int analyse_interval = *((int*)arg);
    while (1)
    {
        sleep(analyse_interval);
        pthread_mutex_lock(&sharedlist.lock);
        // iterate through shared list and compute frequency
        Node* current = sharedlist.head;

        while(current != NULL)
        {
            if (strstr(current->data, searchPattern.pattern) != NULL)
            {
                searchPattern.frequency++;
            }
            current = current->next;
        }
        pthread_mutex_unlock(&sharedlist.lock);

        if (pthread_equal(pthread_self(), searchPattern.output_thread))
        {
            printf("Most frequent occurances of '%s' : %d\n", searchPattern.pattern, searchPattern.frequency);
            // reset variables to be used again
            searchPattern.frequency = 0;
            searchPattern.output_thread = 0;
        }
    }
    return NULL;
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrSize = sizeof(struct sockaddr_in);
    pthread_t thread;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
}