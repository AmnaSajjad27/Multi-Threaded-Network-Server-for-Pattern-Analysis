#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Define server settings
#define PORT 12345
#define MAX_CLIENTS 100

// Define book data structure
struct BookNode {
    char title[100];
    char content[1000];
    struct BookNode* next;
};

// Shared data structure (list) with book pointers
struct BookNode* shared_list = NULL;
struct BookNode* book_heads[MAX_CLIENTS];

// Function to add a book node to the shared list
void add_book_node(char* title, char* content, int client_id) {
    struct BookNode* new_node = (struct BookNode*)malloc(sizeof(struct BookNode));
    strcpy(new_node->title, title);
    strcpy(new_node->content, content);
    new_node->next = NULL;

    if (shared_list == NULL) {
        shared_list = new_node;
    } else {
        struct BookNode* current = shared_list;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }

    if (book_heads[client_id] == NULL) {
        book_heads[client_id] = new_node;
    } else {
        struct BookNode* current = book_heads[client_id];
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
}

// Function to print a book
void print_book(int client_id) {
    struct BookNode* current = book_heads[client_id];
    FILE* file;
    char filename[50];
    sprintf(filename, "book_%02d.txt", client_id);

    if ((file = fopen(filename, "w")) == NULL) {
        perror("Error opening file");
        return;
    }

    while (current != NULL) {
        fprintf(file, "%s\n", current->title);
        current = current->next;
    }

    fclose(file);

    printf("Wrote book for client %d to %s\n", client_id, filename);
}

// Function to handle each client connection
void* handle_client(void* arg) {
    int client_id = *((int*)arg);
    free(arg);

    // Add a book header for this client
    book_heads[client_id] = NULL;

    char buffer[1100];
    memset(buffer, 0, sizeof(buffer));

    while (1) {
        int bytes_received = recv(client_id, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            break;
        }

        // Extract book title and content from the received data
        char title[100], content[1000];
        sscanf(buffer, "%99[^\n]\n%999[^\n]", title, content);

        add_book_node(title, content, client_id);

        printf("Received data from client %d: %s\n", client_id, title);
        memset(buffer, 0, sizeof(buffer));
    }

    // Print the received book
    print_book(client_id);

    close(client_id);
    return NULL;
}

char* search_pattern = "happy"; // Change this to your desired pattern

void* analyze(void* arg) {
    while (1) {
        sleep(5); // Adjust the interval as needed (5 seconds in this example)

        // Count occurrences of search_pattern
        int pattern_count = 0;
        struct BookNode* current = shared_list;
        while (current != NULL) {
            if (strstr(current->content, search_pattern) != NULL) {
                pattern_count++;
            }
            current = current->next;
        }

        // Print result
        printf("Occurrences of '%s': %d\n", search_pattern, pattern_count);
    }

    return NULL;
}

int main() {
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
    server_addr.sin_port = htons(PORT);
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

    printf("Server listening on port %d...\n", PORT);

    // Initialize book_heads array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        book_heads[i] = NULL;
    }

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("Accept failed");
            continue;
        }

        // Create a thread to handle the client
        pthread_t client_thread;
        int* client_id = malloc(sizeof(int));
        *client_id = client_fd;

        if (pthread_create(&client_thread, NULL, handle_client, (void*)client_id) != 0) {
            perror("Thread creation failed");
            close(client_fd);
            free(client_id);
            continue;
        }

        pthread_detach(client_thread);
    }

    // Create analysis threads
    pthread_t analysis_thread1, analysis_thread2;
    pthread_create(&analysis_thread1, NULL, analyze, NULL);
    pthread_create(&analysis_thread2, NULL, analyze, NULL);

    // ... (rest of the previous code remains the same)

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        // ... (same as before)
    }

    close(server_fd);
    return 0;
}