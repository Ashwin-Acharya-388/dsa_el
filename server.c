#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>
#include <math.h>
#include "kdtree.h"

#define PORT 8080
#define BUFFER_SIZE 4096

// Simple JSON response helper
void send_json_response(int client_socket, const char *json) {
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/json\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Content-Length: %ld\r\n"
             "\r\n"
             "%s",
             strlen(json), json);
    
    send(client_socket, response, strlen(response), 0);
}

// Helper to URL decode (simple version)
void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}

// Process API requests
void handle_request(int client_socket, const char *request) {
    // Simple routing
    if (strstr(request, "GET /api/nearest")) {
        // Parse query parameters
        // Example: GET /api/nearest?x=150&y=200&type=all&algorithm=kdtree HTTP/1.1
        
        float x = 0, y = 0;
        char type[50] = "all";
        char algorithm[50] = "kdtree";
        
        // Find the query string start
        const char *query_start = strchr(request, '?');
        if (query_start) {
            // Very basic parsing assuming expected order or simple tokens
            // This is fragile but works for the project demo
            
            // Temporary buffer to work with query string
            char query[1024];
            const char *query_end = strchr(query_start, ' ');
            if (!query_end) query_end = query_start + strlen(query_start);
            
            int len = query_end - (query_start + 1);
            if (len > 1023) len = 1023;
            strncpy(query, query_start + 1, len);
            query[len] = '\0';
            
            // Parse parameters manually
            char *token = strtok(query, "&");
            while (token != NULL) {
                if (strncmp(token, "x=", 2) == 0) {
                    x = atof(token + 2);
                } else if (strncmp(token, "y=", 2) == 0) {
                    y = atof(token + 2);
                } else if (strncmp(token, "type=", 5) == 0) {
                    strncpy(type, token + 5, sizeof(type) - 1);
                } else if (strncmp(token, "algorithm=", 10) == 0) {
                    strncpy(algorithm, token + 10, sizeof(algorithm) - 1);
                }
                token = strtok(NULL, "&");
            }
        }
        
        // Perform search
        int search_count = 0;
        Venue result_venue;
        result_venue.id = -1; // Default not found
        
        if (strcmp(algorithm, "kdtree") == 0) {
            KDNode *best_node = find_nearest_neighbor(kd_tree, x, y, type, &search_count);
            if (best_node) {
                result_venue = best_node->venue;
            }
        } else {
            result_venue = brute_force_nearest(x, y, type, &search_count);
        }
        
        // Create JSON response
        char json[BUFFER_SIZE];
        if (result_venue.id != -1) {
            char complexity[20];
            if (strcmp(algorithm, "kdtree") == 0)
                strcpy(complexity, "O(log n)");
            else
                strcpy(complexity, "O(n)");
                
            float dist = calculate_distance(x, y, result_venue.x, result_venue.y);
            
            snprintf(json, BUFFER_SIZE,
                     "{\"success\": true, "
                     "\"query\": {\"x\": %.1f, \"y\": %.1f, \"type\": \"%s\", \"algorithm\": \"%s\"}, "
                     "\"result\": {\"id\": %d, \"name\": \"%s\", \"type\": \"%s\", "
                     "\"x\": %.1f, \"y\": %.1f, \"distance\": %.1f}, "
                     "\"searches\": %d, "
                     "\"complexity\": \"%s\"}",
                     x, y, type, algorithm,
                     result_venue.id, result_venue.name, result_venue.type, 
                     result_venue.x, result_venue.y, dist,
                     search_count, complexity);
        } else {
            snprintf(json, BUFFER_SIZE,
                     "{\"success\": false, "
                     "\"message\": \"No venue found matching criteria\"}");
        }
        
        send_json_response(client_socket, json);
    }
    else if (strstr(request, "GET /api/venues")) {
        // Return all venues
        char json[BUFFER_SIZE * 4]; // Larger buffer for all venues
        strcpy(json, "{\"venues\": [");
        
        for (int i = 0; i < venue_count; i++) {
            char venue_json[256];
            snprintf(venue_json, sizeof(venue_json),
                     "{\"id\": %d, \"name\": \"%s\", \"type\": \"%s\", \"x\": %.1f, \"y\": %.1f}%s",
                     venues[i].id, venues[i].name, venues[i].type, venues[i].x, venues[i].y,
                     (i < venue_count - 1) ? ", " : "");
            strcat(json, venue_json);
        }
        strcat(json, "]}");
        
        send_json_response(client_socket, json);
    }
    else {
        // Return 404 for unknown routes
        char response[] = "HTTP/1.1 404 Not Found\r\n"
                         "Content-Type: text/plain\r\n"
                         "\r\n"
                         "404 - API endpoint not found";
        send(client_socket, response, strlen(response), 0);
    }
}

// Client handler thread
void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);
    
    char buffer[BUFFER_SIZE];
    int bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        handle_request(client_socket, buffer);
    }
    
    close(client_socket);
    return NULL;
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Initialize Data
    initialize_sample_venues();
    kd_tree = build_kdtree(venues, venue_count, 0);
    printf("Backend Initialized: %d venues loaded, K-D Tree built.\n", venue_count);
    
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure socket
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("C Backend Server running on port %d\n", PORT);
    printf("Connect frontend to http://localhost:%d\n", PORT);
    
    // Main server loop
    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        // Create thread for client
        int *client_ptr = malloc(sizeof(int));
        *client_ptr = client_socket;
        
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, client_ptr) != 0) {
            perror("Thread creation failed");
            free(client_ptr);
            close(client_socket);
        } else {
            pthread_detach(thread);
        }
    }
    
    return 0;
}