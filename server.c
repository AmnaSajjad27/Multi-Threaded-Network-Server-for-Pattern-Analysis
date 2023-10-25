#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Define server settings
#define MAX_CLIENTS 100

// Define book data structure
struct BookNode 
{
    char title[200];
    // changed this array to a pointer for dynamic allocation
    char* content;
    struct BookNode* next;
    struct BookNode* book_next;
};

// Shared data structure (list) with book pointers
struct BookNode* shared_list = NULL;
struct BookNode* book_heads[MAX_CLIENTS];
int client_count = 0;

// Define and initalise search pattern 
char* search_pattern = NULL;

// initalise mutex for locks
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// global varibale to count file names 
int client_id_counter = 0;

int get_client_id()
{
    pthread_mutex_lock(&mutex);
    int id = client_id_counter++;
    pthread_mutex_unlock(&mutex);
    return id;
}

// Function to add a book node to the shared list
void add_book_node(char* title, char* content, int client_id) 
{
    struct BookNode* new_node = (struct BookNode*)malloc(sizeof(struct BookNode));
    strcpy(new_node->title, title);

    // dynamic allocation
    new_node->content = (char*)malloc(strlen(content) + 1);
    strcpy(new_node->content, content);

    new_node->next = NULL;
    new_node->book_next = NULL;

    if (shared_list == NULL) 
    {
        shared_list = new_node;
    }
    else
    {
        struct BookNode* current = shared_list;

        while (current->next != NULL) 
        {
            current = current->next;
        }
        current->next = new_node;
    }

    if (book_heads[client_id] == NULL) 
    {
        book_heads[client_id] = new_node;
    }
    else
    {
        struct BookNode* current = book_heads[client_id];

        while (current->book_next != NULL)
        {
            current = current->book_next;
        }
        current->book_next = new_node;
    }
    printf("Added node for client %02d: %s\n", client_id, title);

}

// Function to print a book
void print_book(int client_id, int connection_order)
{
    struct BookNode* current = book_heads[client_id];
    FILE* file;
    char filename[50];

    // -1 to start printing from 00 not 01 as its incremented later
    sprintf(filename, "book_%02d.txt", connection_order - 1);  // Use %02d for leading zeros

    if ((file = fopen(filename, "w")) == NULL) 
    {
        perror("Error opening file");
        return;
    }

    while (current != NULL) 
    {
        fprintf(file, "%s\n%s\n", current->title, current->content);
        current = current->next;
    }

    fclose(file);

    printf("Wrote book for client %02d to %s\n", client_id , filename);
}

// Function to handle each client connection
void* handle_client(void* arg) 
{
    int client_id = *((int*)arg);
    free(arg);

    client_count++;;

    // Add a book header for this client
    book_heads[client_id] = NULL;

    char buffer[1100];
    memset(buffer, 0, sizeof(buffer));

    while (1) 
    {
        int bytes_received = recv(client_id, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0)
        {
            break;
        }

        // get book title and content from the received data
        char title[100], content[1000];
        sscanf(buffer, "%99[^\n]\n%999[^\n]", title, content);

        // Lock the thread, then add the book
        pthread_mutex_lock(&mutex);
        add_book_node(title, content, client_id);
        pthread_mutex_unlock(&mutex);

        add_book_node(title, content, client_id);

        memset(buffer, 0, sizeof(buffer));
    }

    // Print the received book
    print_book(client_id, client_count);

    close(client_id);
    client_count--;

    if (client_count == 0)
    {
        pthread_mutex_lock(&mutex);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

//  Function to analyse given a string arg
void* analyze(void* arg) 
{
    while (1) 
    {
        sleep(5); // interval

        // Count occurrences of the given search_pattern
        int pattern_count = 0;
        struct BookNode* current = shared_list;

        while (current != NULL) 
        {
            if (strstr(current->content, search_pattern) != NULL) 
            {
                pattern_count++;
            }
            current = current->next;
        }

        // Print occurences 
        printf("Occurrences of '%s': %d\n", search_pattern, pattern_count);
    }

    return NULL;
}


int main(int argc, char* argv[])
{

    // default port
    int listen_port = 12345;

    if (argc == 5 && strcmp(argv[1], "-l") == 0 && strcmp(argv[3], "-p") == 0) 
    {
        listen_port = atoi(argv[2]);
        search_pattern = argv[4];
    }
    else
    {
        printf("Usage: %s -l <port> -p <search_pattern>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(listen_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", listen_port);

    struct sockaddr_in temp_addr;
    socklen_t temp_len = sizeof(temp_addr);
    getsockname(server_fd, (struct sockaddr*)&temp_addr, &temp_len);

    // Initialize book_heads array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        book_heads[i] = NULL;
    }

    // Initialize mutex
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(EXIT_FAILURE);
    }

    pthread_t client_thread;

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("Accept failed");
            continue;
        }

        // Create a thread to handle the client
        int* client_id = malloc(sizeof(int));
        *client_id = get_client_id();

        if (pthread_create(&client_thread, NULL, handle_client, (void*)client_id) != 0) {
            perror("Thread creation failed");
            close(client_fd);
            free(client_id);
            continue;
        }

        pthread_detach(client_thread);
    }

    // Create analysis threads
    pthread_mutex_destroy(&mutex);

    close(server_fd);
    return 0;
}