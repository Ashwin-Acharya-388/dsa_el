#ifndef KDTREE_H
#define KDTREE_H

#define MAX_VENUES 500
#define MAX_NAME_LENGTH 50
#define MAX_TYPE_LENGTH 20

// Structure for a venue
typedef struct {
    int id;
    char name[MAX_NAME_LENGTH];
    char type[MAX_TYPE_LENGTH];
    float x;
    float y;
    char color[20];
    char price[15];
} Venue;

// Structure for K-D Tree node
typedef struct KDNode {
    Venue venue;
    int depth;
    struct KDNode *left;
    struct KDNode *right;
} KDNode;

// Global variables (defined in backend.c)
extern Venue venues[MAX_VENUES];
extern int venue_count;
extern KDNode *kd_tree;

// Function prototypes
KDNode* create_kdnode(Venue venue, int depth);
KDNode* build_kdtree(Venue venues[], int n, int depth);
void free_kdtree(KDNode *node);
KDNode* find_nearest_neighbor(KDNode *root, float x, float y, char *type, int *search_count);
void nearest_search(KDNode *node, float x, float y, int depth, KDNode **best, float *best_dist, char *type, int *search_count);
float calculate_distance(float x1, float y1, float x2, float y2);
Venue brute_force_nearest(float x, float y, char *type, int *search_count);
void print_tree(KDNode *node, int level);
void initialize_sample_venues();
int add_venue_backend(const char* name, const char* type, float x, float y);
void get_tree_string(KDNode *node, char *buffer, int *pos, int max_len, int level);

#endif // KDTREE_H
