#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
#include <ctype.h>
#include <pthread.h>
#include <math.h>
#include "kdtree.h"

#define PORT 8080
#define BUFFER_SIZE 4096

// Simple JSON response helper  
void send_json_response(int client_socket, const char *json) {
    char header[512];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/json\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Content-Length: %ld\r\n"
             "\r\n",
             (long)strlen(json));
    
    // Send header
    send(client_socket, header, strlen(header), 0);
    // Send body
    send(client_socket, json, strlen(json), 0);
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
    // Handle different routes
    if (strstr(request, "GET / ") || strstr(request, "GET /index.html")) {
        // Serve landing page
        send_file_response(client_socket, "index.html", "text/html");
    }
    else if (strstr(request, "GET /map.html")) {
        // Serve map interface
        send_file_response(client_socket, "map.html", "text/html");
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
    else if (strstr(request, "POST /api/venue")) {
        // Parse JSON body
        // Find double CRLF which marks end of headers
        const char *body = strstr(request, "\r\n\r\n");
        if (body) {
            body += 4; // Skip CRLF
            
            char name[100] = "";
            char type[50] = "";
            float x = 0, y = 0;
            
            // Simple JSON parsing (expecting {"name":"...","type":"...","x":...,"y":...})
            // We'll use strstr to find keys
            
            char *p_name = strstr(body, "\"name\":");
            if (p_name) {
                p_name = strchr(p_name, ':');
                if (p_name) {
                    p_name = strchr(p_name, '"');
                    if (p_name) {
                        p_name++; // Skip quote
                        char *end = strchr(p_name, '"');
                        if (end) {
                            int len = end - p_name;
                            if (len < sizeof(name)) {
                                strncpy(name, p_name, len);
                                name[len] = '\0';
                            }
                        }
                    }
                }
            }
            
            char *p_type = strstr(body, "\"type\":");
            if (p_type) {
                p_type = strchr(p_type, ':');
                if (p_type) {
                    p_type = strchr(p_type, '"');
                    if (p_type) {
                        p_type++; // Skip quote
                        char *end = strchr(p_type, '"');
                        if (end) {
                            int len = end - p_type;
                            if (len < sizeof(type)) {
                                strncpy(type, p_type, len);
                                type[len] = '\0';
                            }
                        }
                    }
                }
            }
            
            char *p_x = strstr(body, "\"x\":");
            if (p_x) {
                p_x = strchr(p_x, ':');
                if (p_x) x = atof(p_x + 1);
            }
            
            char *p_y = strstr(body, "\"y\":");
            if (p_y) {
                p_y = strchr(p_y, ':');
                if (p_y) y = atof(p_y + 1);
            }
            
            if (strlen(name) > 0) {
                int id = add_venue_backend(name, type, x, y);
                if (id > 0) {
                     char json[512];
                     snprintf(json, sizeof(json), 
                         "{\"success\": true, \"message\": \"Venue added\", \"id\": %d}", id);
                     send_json_response(client_socket, json);
                } else {
                     send_json_response(client_socket, "{\"success\": false, \"message\": \"Failed to add venue (Limit reached)\"}");
                }
            } else {
                 send_json_response(client_socket, "{\"success\": false, \"message\": \"Invalid JSON data\"}");
            }
        } else {
            send_json_response(client_socket, "{\"success\": false, \"message\": \"No body found\"}");
        }
    }
    else if (strstr(request, "GET /api/venues")) {
        // Return all venues
        // Estimate size: 500 venues * ~150 bytes = ~75KB
        int estimated_size = venue_count * 200 + 1024;
        char *json = malloc(estimated_size);
        if (json) {
            strcpy(json, "{\"venues\": [");
            
            for (int i = 0; i < venue_count; i++) {
                char venue_json[256];
                // Clean price string (remove newlines if any)
                char clean_price[20];
                strncpy(clean_price, venues[i].price, sizeof(clean_price)-1);
                clean_price[sizeof(clean_price)-1] = '\0';
                char *newline = strchr(clean_price, '\n');
                if (newline) *newline = '\0';

                snprintf(venue_json, sizeof(venue_json),
                         "{\"id\": %d, \"name\": \"%s\", \"type\": \"%s\", \"x\": %.4f, \"y\": %.4f, \"color\": \"%s\", \"price\": \"%s\"}%s",
                         venues[i].id, venues[i].name, venues[i].type, venues[i].x, venues[i].y, 
                         venues[i].color, clean_price,
                         (i < venue_count - 1) ? ", " : "");
                strcat(json, venue_json);
            }
            strcat(json, "]}");
            
            send_json_response(client_socket, json);
            free(json);
        } else {
             send_json_response(client_socket, "{\"error\": \"Memory allocation failed\"}");
        }
    }
    else if (strstr(request, "GET /api/tree")) {
        // Return tree visualization string
        // Max venues 500 * ~100 bytes/node = ~50KB
        char *tree_buffer = calloc(1, 65536); // 64KB buffer
        if (tree_buffer) {
            int pos = 0;
            get_tree_string(kd_tree, tree_buffer, &pos, 65536, 0);
            
            // Allocate json buffer (double size to be safe with escaping if needed, 
            // though we don't escape much here)
            char *json = malloc(65536 + 1024);
            if (json) {
                // We need to be careful with JSON escaping. 
                // Simple version: just wrap it. 
                // But the tree string has newlines and quotes.
                // Let's use a very simple escape for newlines.
                
                strcpy(json, "{\"treeStructure\": \"");
                char *p_json = json + strlen(json);
                char *p_tree = tree_buffer;
                
                while (*p_tree && (p_json - json < 65536 + 500)) {
                    if (*p_tree == '\n') {
                        *p_json++ = '\\';
                        *p_json++ = 'n';
                    } else if (*p_tree == '"') {
                        *p_json++ = '\\';
                        *p_json++ = '"';
                    } else {
                        *p_json++ = *p_tree;
                    }
                    p_tree++;
                }
                strcpy(p_json, "\"}");
                
                send_json_response(client_socket, json);
                free(json);
            }
            free(tree_buffer);
        } else {
             send_json_response(client_socket, "{\"error\": \"Buffer allocation failed\"}");
        }
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

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\\n");
        return 1;
    }
#endif
    
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
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt))) {
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