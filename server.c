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

// Serve static file
void send_file_response(int client_socket, const char *filepath, const char *content_type) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        char response[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nFile not found";
        send(client_socket, response, strlen(response), 0);
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = malloc(file_size + 1);
    if (!file_content) {
        fclose(file);
        return;
    }
    
    fread(file_content, 1, file_size, file);
    fclose(file);
    file_content[file_size] = '\0';

    char header[1024];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "\r\n",
             content_type, file_size);
    
    send(client_socket, header, strlen(header), 0);
    send(client_socket, file_content, file_size, 0);
    
    free(file_content);
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
    if (strstr(request, "GET / ") || strstr(request, "GET /index.html ")) {
        send_file_response(client_socket, "index.html", "text/html");
    }
    else if (strstr(request, "GET /api/nearest")) {
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
        // Return all venues with favorite status
        char json[BUFFER_SIZE * 4]; // Larger buffer for all venues
        strcpy(json, "{\"venues\": [");
        
        for (int i = 0; i < venue_count; i++) {
            char venue_json[256];
            snprintf(venue_json, sizeof(venue_json),
                     "{\"id\": %d, \"name\": \"%s\", \"type\": \"%s\", \"x\": %.4f, \"y\": %.4f, \"is_favorite\": %d}%s",
                     venues[i].id, venues[i].name, venues[i].type, venues[i].x, venues[i].y,
                     venues[i].is_favorite,
                     (i < venue_count - 1) ? ", " : "");
            strcat(json, venue_json);
        }
        strcat(json, "]}");
        
        send_json_response(client_socket, json);
    }
    else if (strstr(request, "GET /api/favorites")) {
        // Return all favorite venues
        char json[BUFFER_SIZE * 4];
        Venue favorites[MAX_VENUES];
        int fav_count = 0;
        
        get_favorites_list(favorites, &fav_count);
        
        strcpy(json, "{\"success\": true, \"favorites\": [");
        
        for (int i = 0; i < fav_count; i++) {
            char fav_json[256];
            snprintf(fav_json, sizeof(fav_json),
                     "{\"id\": %d, \"name\": \"%s\", \"type\": \"%s\", \"x\": %.4f, \"y\": %.4f}%s",
                     favorites[i].id, favorites[i].name, favorites[i].type, 
                     favorites[i].x, favorites[i].y,
                     (i < fav_count - 1) ? ", " : "");
            strcat(json, fav_json);
        }
        strcat(json, "]}");
        
        send_json_response(client_socket, json);
    }
    else if (strstr(request, "GET /api/toggle-favorite")) {
        // Parse venue ID from query parameter
        int venue_id = 0;
        const char *query_start = strchr(request, '?');
        if (query_start) {
            char query[256];
            const char *query_end = strchr(query_start, ' ');
            if (!query_end) query_end = query_start + strlen(query_start);
            
            int len = query_end - (query_start + 1);
            if (len > 255) len = 255;
            strncpy(query, query_start + 1, len);
            query[len] = '\0';
            
            // Parse id parameter
            char *token = strtok(query, "&");
            while (token != NULL) {
                if (strncmp(token, "id=", 3) == 0) {
                    venue_id = atoi(token + 3);
                }
                token = strtok(NULL, "&");
            }
        }
        
        if (venue_id > 0) {
            toggle_favorite(venue_id);
            
            // Find the toggled venue to return its new status
            int is_fav = 0;
            for (int i = 0; i < venue_count; i++) {
                if (venues[i].id == venue_id) {
                    is_fav = venues[i].is_favorite;
                    break;
                }
            }
            
            char json[256];
            snprintf(json, sizeof(json),
                     "{\"success\": true, \"venue_id\": %d, \"is_favorite\": %d}",
                     venue_id, is_fav);
            send_json_response(client_socket, json);
        } else {
            char json[] = "{\"success\": false, \"message\": \"Invalid venue ID\"}";
            send_json_response(client_socket, json);
        }
    }
    else if (strstr(request, "GET /api/nearby-favorites")) {
        // Parse query parameters: x, y, radius
        float x = 0, y = 0, radius = 1.0;
        const char *query_start = strchr(request, '?');
        if (query_start) {
            char query[1024];
            const char *query_end = strchr(query_start, ' ');
            if (!query_end) query_end = query_start + strlen(query_start);
            
            int len = query_end - (query_start + 1);
            if (len > 1023) len = 1023;
            strncpy(query, query_start + 1, len);
            query[len] = '\0';
            
            char *token = strtok(query, "&");
            while (token != NULL) {
                if (strncmp(token, "x=", 2) == 0) {
                    x = atof(token + 2);
                } else if (strncmp(token, "y=", 2) == 0) {
                    y = atof(token + 2);
                } else if (strncmp(token, "radius=", 7) == 0) {
                    radius = atof(token + 7);
                }
                token = strtok(NULL, "&");
            }
        }
        
        Venue nearby[MAX_VENUES];
        int nearby_count = 0;
        get_nearby_favorites(x, y, radius, nearby, &nearby_count);
        
        char json[BUFFER_SIZE * 4];
        strcpy(json, "{\"success\": true, \"nearby_favorites\": [");
        
        for (int i = 0; i < nearby_count; i++) {
            float dist = calculate_distance(x, y, nearby[i].x, nearby[i].y);
            char fav_json[512];
            snprintf(fav_json, sizeof(fav_json),
                     "{\"id\": %d, \"name\": \"%s\", \"type\": \"%s\", \"x\": %.4f, \"y\": %.4f, \"distance\": %.2f}%s",
                     nearby[i].id, nearby[i].name, nearby[i].type, 
                     nearby[i].x, nearby[i].y, dist,
                     (i < nearby_count - 1) ? ", " : "");
            strcat(json, fav_json);
        }
        strcat(json, "], ");
        
        char info[256];
        snprintf(info, sizeof(info), 
                 "\"query\": {\"x\": %.4f, \"y\": %.4f, \"radius\": %.2f}, \"count\": %d}",
                 x, y, radius, nearby_count);
        strcat(json, info);
        
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
