#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "kdtree.h"

// Global arrays for venues and tree
Venue venues[MAX_VENUES];
int venue_count = 0;
KDNode *kd_tree = NULL;

// Create a new K-D Tree node
KDNode* create_kdnode(Venue venue, int depth) {
    KDNode *node = (KDNode*)malloc(sizeof(KDNode));
    node->venue = venue;
    node->depth = depth;
    node->left = NULL;
    node->right = NULL;
    return node;
}

// Calculate Euclidean distance
float calculate_distance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

// Compare function for qsort based on axis
int compare_x(const void *a, const void *b) {
    Venue *va = (Venue*)a;
    Venue *vb = (Venue*)b;
    if (va->x < vb->x) return -1;
    if (va->x > vb->x) return 1;
    return 0;
}

int compare_y(const void *a, const void *b) {
    Venue *va = (Venue*)a;
    Venue *vb = (Venue*)b;
    if (va->y < vb->y) return -1;
    if (va->y > vb->y) return 1;
    return 0;
}

// Build K-D Tree recursively
KDNode* build_kdtree(Venue venues[], int n, int depth) {
    if (n <= 0) return NULL;
    
    int axis = depth % 2;
    
    // Sort based on current axis
    if (axis == 0) {
        qsort(venues, n, sizeof(Venue), compare_x);
    } else {
        qsort(venues, n, sizeof(Venue), compare_y);
    }
    
    int median = n / 2;
    KDNode *node = create_kdnode(venues[median], depth);
    
    // Build left and right subtrees
    if (median > 0) {
        node->left = build_kdtree(venues, median, depth + 1);
    }
    if (median + 1 < n) {
        node->right = build_kdtree(venues + median + 1, n - median - 1, depth + 1);
    }
    
    return node;
}

// Recursive nearest neighbor search
void nearest_search(KDNode *node, float x, float y, int depth, 
                    KDNode **best, float *best_dist, char *type, int *search_count) {
    if (node == NULL) return;
    
    (*search_count)++;
    
    // Check if this node matches the type filter
    int type_matches = (strcmp(type, "all") == 0) || (strcmp(node->venue.type, type) == 0);
    
    // Calculate distance to this node's venue
    float dist = calculate_distance(x, y, node->venue.x, node->venue.y);
    
    // Update best if this node is closer and matches type
    if (dist < *best_dist && type_matches) {
        *best = node;
        *best_dist = dist;
    }
    
    // Determine axis to compare
    int axis = depth % 2;
    float point_coord = (axis == 0) ? x : y;
    float node_coord = (axis == 0) ? node->venue.x : node->venue.y;
    
    // Decide which subtree to search first
    KDNode *nearer_subtree = NULL;
    KDNode *farther_subtree = NULL;
    
    if (point_coord < node_coord) {
        nearer_subtree = node->left;
        farther_subtree = node->right;
    } else {
        nearer_subtree = node->right;
        farther_subtree = node->left;
    }
    
    // Search the nearer subtree first
    nearest_search(nearer_subtree, x, y, depth + 1, best, best_dist, type, search_count);
    
    // Check if we need to search the farther subtree
    float plane_distance = fabs(point_coord - node_coord);
    if (plane_distance < *best_dist) {
        nearest_search(farther_subtree, x, y, depth + 1, best, best_dist, type, search_count);
    }
}

// Find nearest neighbor using K-D Tree
KDNode* find_nearest_neighbor(KDNode *root, float x, float y, char *type, int *search_count) {
    if (root == NULL) return NULL;
    
    *search_count = 0;
    KDNode *best = NULL;
    float best_dist = INFINITY;
    
    nearest_search(root, x, y, 0, &best, &best_dist, type, search_count);
    return best;
}

// Brute force nearest neighbor search for comparison
Venue brute_force_nearest(float x, float y, char *type, int *search_count) {
    Venue best_venue;
    float best_dist = INFINITY;
    int found = 0;
    *search_count = 0;
    
    for (int i = 0; i < venue_count; i++) {
        (*search_count)++;
        
        if (strcmp(type, "all") != 0 && strcmp(venues[i].type, type) != 0) {
            continue;
        }
        
        float dist = calculate_distance(x, y, venues[i].x, venues[i].y);
        if (dist < best_dist) {
            best_dist = dist;
            best_venue = venues[i];
            found = 1;
        }
    }
    
    if (!found) {
        // Return empty venue if none found
        best_venue.id = -1;
    }
    
    return best_venue;
}

// Print tree structure (for debugging/visualization)
void print_tree(KDNode *node, int level) {
    if (node == NULL) return;
    
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
    
    printf("├── %s (%c) [%.1f, %.1f] depth:%d\n", 
           node->venue.name, 
           node->venue.type[0],
           node->venue.x,
           node->venue.y,
           node->depth);
    
    print_tree(node->left, level + 1);
    print_tree(node->right, level + 1);
}

// Free K-D Tree memory
void free_kdtree(KDNode *node) {
    if (node == NULL) return;
    
    free_kdtree(node->left);
    free_kdtree(node->right);
    free(node);
}

// Initialize sample venues
void initialize_sample_venues() {
    venue_count = 15;
    
    // Sample venues data
    Venue sample_venues[15] = {
        {1, "Cineplex City", "cinema", 150, 120, "#ff6b6b"},
        {2, "Foodie Paradise", "restaurant", 300, 80, "#4ecdc4"},
        {3, "Central Park", "park", 500, 200, "#ffe66d"},
        {4, "Mega Mall", "mall", 250, 300, "#ff9a76"},
        {5, "Grand Theater", "theater", 600, 350, "#a78bfa"},
        {6, "Starlight Cinema", "cinema", 700, 150, "#ff6b6b"},
        {7, "Gourmet Corner", "restaurant", 100, 400, "#4ecdc4"},
        {8, "Riverside Park", "park", 400, 450, "#ffe66d"},
        {9, "Plaza Mall", "mall", 650, 250, "#ff9a76"},
        {10, "Drama House", "theater", 200, 200, "#a78bfa"},
        {11, "Cinema Royale", "cinema", 350, 150, "#ff6b6b"},
        {12, "Italian Bistro", "restaurant", 550, 100, "#4ecdc4"},
        {13, "Botanical Gardens", "park", 150, 250, "#ffe66d"},
        {14, "Fashion Mall", "mall", 450, 350, "#ff9a76"},
        {15, "Opera Hall", "theater", 750, 400, "#a78bfa"}
    };
    
    // Copy sample venues to global array
    for (int i = 0; i < venue_count; i++) {
        venues[i] = sample_venues[i];
    }
}

// Main function guard for CLI mode
#ifdef CLI_MODE
int main() {
    initialize_sample_venues();
    
    printf("=== 2D Tree Nearest Neighbor Search Backend ===\n");
    printf("Initialized with %d entertainment venues\n\n", venue_count);
    
    // Build K-D Tree
    kd_tree = build_kdtree(venues, venue_count, 0);
    printf("K-D Tree built successfully!\n\n");
    
    printf("Tree Structure:\n");
    print_tree(kd_tree, 0);
    printf("\n");
    
    // Test search
    float test_x = 400, test_y = 250;
    char test_type[] = "all";
    
    printf("Test search at point (%.1f, %.1f):\n", test_x, test_y);
    
    // K-D Tree search
    int kd_search_count;
    KDNode *kd_result = find_nearest_neighbor(kd_tree, test_x, test_y, test_type, &kd_search_count);
    
    if (kd_result != NULL) {
        printf("K-D Tree found: %s (%s) at (%.1f, %.1f)\n", 
               kd_result->venue.name, kd_result->venue.type,
               kd_result->venue.x, kd_result->venue.y);
        printf("K-D Tree searches: %d\n", kd_search_count);
    }
    
    // Brute force search
    int bf_search_count;
    Venue bf_result = brute_force_nearest(test_x, test_y, test_type, &bf_search_count);
    
    if (bf_result.id != -1) {
        printf("Brute Force found: %s (%s) at (%.1f, %.1f)\n", 
               bf_result.name, bf_result.type, bf_result.x, bf_result.y);
        printf("Brute Force searches: %d\n", bf_search_count);
    }
    
    printf("\nEfficiency Comparison:\n");
    printf("K-D Tree: O(log n) = %d operations\n", kd_search_count);
    printf("Brute Force: O(n) = %d operations\n", bf_search_count);
    printf("Speedup: %.2fx faster\n", (float)bf_search_count / kd_search_count);
    
    // Clean up
    free_kdtree(kd_tree);
    
    return 0;
}
#endif