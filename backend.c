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
    venue_count = 100;
    
    // Sample venues data (Real Bangalore Locations)
    Venue sample_venues[100] = {
        // ---------------- HOSPITALS (1–20) ----------------
        {1,"Manipal Hospital Whitefield","hospital",12.9698,77.7500,"#e74c3c"},
        {2,"Apollo Hospital Bannerghatta","hospital",12.8449,77.6034,"#e74c3c"},
        {3,"Fortis Hospital Bannerghatta","hospital",12.8076,77.5993,"#e74c3c"},
        {4,"Narayana Health City","hospital",12.8069,77.5849,"#e74c3c"},
        {5,"Columbia Asia Hebbal","hospital",13.0358,77.5970,"#e74c3c"},
        {6,"St Johns Medical College","hospital",12.9308,77.6205,"#e74c3c"},
        {7,"Victoria Hospital","hospital",12.9636,77.5731,"#e74c3c"},
        {8,"Bowring Hospital","hospital",12.9816,77.6030,"#e74c3c"},
        {9,"Ramaiah Memorial Hospital","hospital",13.0296,77.5646,"#e74c3c"},
        {10,"Aster CMI Hospital","hospital",13.0595,77.5937,"#e74c3c"},
        {11,"Sakra World Hospital","hospital",12.9255,77.6846,"#e74c3c"},
        {12,"BGS Gleneagles Hospital","hospital",12.9077,77.4856,"#e74c3c"},
        {13,"People Tree Hospital","hospital",13.0225,77.5207,"#e74c3c"},
        {14,"ESI Hospital Rajajinagar","hospital",12.9915,77.5531,"#e74c3c"},
        {15,"KC General Hospital","hospital",12.9966,77.5706,"#e74c3c"},
        {16,"Cloudnine Hospital Jayanagar","hospital",12.9302,77.5833,"#e74c3c"},
        {17,"NIMHANS","hospital",12.9435,77.5967,"#e74c3c"},
        {18,"Vydehi Hospital","hospital",12.9731,77.7273,"#e74c3c"},
        {19,"Chinmaya Mission Hospital","hospital",12.9837,77.6605,"#e74c3c"},
        {20,"Sapthagiri Hospital","hospital",13.0564,77.5136,"#e74c3c"},

        // ---------------- PARKS (21–35) ----------------
        {21,"Cubbon Park","park",12.9762,77.5993,"#27ae60"},
        {22,"Lalbagh Botanical Garden","park",12.9507,77.5848,"#27ae60"},
        {23,"Bannerghatta National Park","park",12.7972,77.5773,"#27ae60"},
        {24,"Ulsoor Lake","park",12.9813,77.6081,"#27ae60"},
        {25,"Sankey Tank","park",12.9899,77.5615,"#27ae60"},
        {26,"JP Park","park",13.0316,77.5610,"#27ae60"},
        {27,"Bugle Rock Park","park",12.9432,77.5711,"#27ae60"},
        {28,"Hesaraghatta Lake","park",13.1391,77.4782,"#27ae60"},
        {29,"Agara Lake","park",12.9204,77.6414,"#27ae60"},
        {30,"Kaikondrahalli Lake","park",12.9116,77.6649,"#27ae60"},
        {31,"Freedom Park","park",12.9833,77.5865,"#27ae60"},
        {32,"Lumbini Gardens","park",13.0603,77.5870,"#27ae60"},
        {33,"Turahalli Forest","park",12.8883,77.5634,"#27ae60"},
        {34,"Yelahanka Lake","park",13.1007,77.5963,"#27ae60"},
        {35,"Madiwala Lake","park",12.9209,77.6173,"#27ae60"},

        // ---------------- CINEMAS (36–50) ----------------
        {36,"PVR Orion Mall","cinema",13.0107,77.5547,"#9b59b6"},
        {37,"INOX Garuda Mall","cinema",12.9716,77.6033,"#9b59b6"},
        {38,"PVR Forum Mall","cinema",12.9279,77.6271,"#9b59b6"},
        {39,"Cinepolis Whitefield","cinema",12.9950,77.7486,"#9b59b6"},
        {40,"PVR Phoenix MarketCity","cinema",12.9698,77.6469,"#9b59b6"},
        {41,"INOX Central Mall","cinema",12.9738,77.6205,"#9b59b6"},
        {42,"PVR Vega City","cinema",12.9071,77.5963,"#9b59b6"},
        {43,"Cinepolis ETA Mall","cinema",13.0214,77.5186,"#9b59b6"},
        {44,"PVR Soul Spirit","cinema",12.9158,77.6101,"#9b59b6"},
        {45,"Gopalan Cinemas","cinema",12.9982,77.7016,"#9b59b6"},
        {46,"INOX Brookefield","cinema",12.9670,77.7156,"#9b59b6"},
        {47,"PVR VR Mall","cinema",13.0536,77.6044,"#9b59b6"},
        {48,"Cinepolis Royal Meenakshi","cinema",12.8804,77.5975,"#9b59b6"},
        {49,"INOX Forum Value Mall","cinema",12.9377,77.5283,"#9b59b6"},
        {50,"PVR MarketSquare","cinema",12.9123,77.5201,"#9b59b6"},

        // ---------------- RESTAURANTS (51–65) ----------------
        {51,"Toit Indiranagar","restaurant",12.9719,77.6412,"#f39c12"},
        {52,"Vidyarthi Bhavan","restaurant",12.9426,77.5669,"#f39c12"},
        {53,"MTR Lalbagh","restaurant",12.9591,77.5857,"#f39c12"},
        {54,"Koshy's","restaurant",12.9716,77.5946,"#f39c12"},
        {55,"The Only Place","restaurant",12.9733,77.6117,"#f39c12"},
        {56,"CTR Malleshwaram","restaurant",13.0037,77.5643,"#f39c12"},
        {57,"Empire Restaurant","restaurant",12.9756,77.6044,"#f39c12"},
        {58,"Truffles Koramangala","restaurant",12.9352,77.6143,"#f39c12"},
        {59,"Nagarjuna Residency Road","restaurant",12.9723,77.5991,"#f39c12"},
        {60,"Meghana Foods","restaurant",12.9355,77.6148,"#f39c12"},
        {61,"Byg Brewski","restaurant",12.9955,77.7056,"#f39c12"},
        {62,"AB's Barbecue","restaurant",12.9701,77.6370,"#f39c12"},
        {63,"Ironhill Brewery","restaurant",12.9111,77.6419,"#f39c12"},
        {64,"Windmills Craftworks","restaurant",12.9726,77.7294,"#f39c12"},
        {65,"Rameshwaram Cafe","restaurant",12.9704,77.5940,"#f39c12"},

        // ---------------- METRO STATIONS (66–100) ----------------
        {66,"MG Road Metro","metro",12.9758,77.6033,"#34495e"},
        {67,"Indiranagar Metro","metro",12.9784,77.6387,"#34495e"},
        {68,"Majestic Metro","metro",12.9767,77.5713,"#34495e"},
        {69,"Yeshwantpur Metro","metro",13.0216,77.5544,"#34495e"},
        {70,"Rajajinagar Metro","metro",12.9918,77.5546,"#34495e"},
        {71,"Jayanagar Metro","metro",12.9304,77.5802,"#34495e"},
        {72,"Banashankari Metro","metro",12.9157,77.5739,"#34495e"},
        {73,"Nagasandra Metro","metro",13.0452,77.5004,"#34495e"},
        {74,"Peenya Metro","metro",13.0324,77.5258,"#34495e"},
        {75,"KR Market Metro","metro",12.9634,77.5786,"#34495e"},
        {76,"Vijayanagar Metro","metro",12.9707,77.5371,"#34495e"},
        {77,"Baiyappanahalli Metro","metro",12.9902,77.6522,"#34495e"},
        {78,"Whitefield Kadugodi Metro","metro",12.9964,77.7597,"#34495e"},
        {79,"Garudacharpalya Metro","metro",12.9851,77.7125,"#34495e"},
        {80,"Halasuru Metro","metro",12.9776,77.6186,"#34495e"},
        {81,"Trinity Metro","metro",12.9749,77.6196,"#34495e"},
        {82,"Cubbon Park Metro","metro",12.9766,77.5933,"#34495e"},
        {83,"Sir M Visvesvaraya Metro","metro",12.9730,77.5930,"#34495e"},
        {84,"Vidhana Soudha Metro","metro",12.9795,77.5907,"#34495e"},
        {85,"Attiguppe Metro","metro",12.9574,77.5286,"#34495e"},
        {86,"Deepanjali Nagar Metro","metro",12.9550,77.5316,"#34495e"},
        {87,"Magadi Road Metro","metro",12.9750,77.5444,"#34495e"},
        {88,"Sandal Soap Factory Metro","metro",13.0075,77.5408,"#34495e"},
        {89,"Dasarahalli Metro","metro",13.0412,77.5125,"#34495e"},
        {90,"Yelachenahalli Metro","metro",12.8914,77.5665,"#34495e"},
        {91,"Konanakunte Cross Metro","metro",12.8847,77.5730,"#34495e"},
        {92,"Vajarahalli Metro","metro",12.8753,77.5647,"#34495e"},
        {93,"Thalaghattapura Metro","metro",12.8691,77.5612,"#34495e"},
        {94,"Silk Institute Metro","metro",12.8615,77.5577,"#34495e"},
        {95,"Nadaprabhu Kempegowda Station","metro",12.9766,77.5713,"#34495e"},
        {96,"Kengeri Metro","metro",12.9127,77.4834,"#34495e"},
        {97,"Pattanagere Metro","metro",12.9234,77.5122,"#34495e"},
        {98,"RV Road Metro","metro",12.9219,77.5800,"#34495e"},
        {99,"BTM Layout Metro","metro",12.9166,77.6101,"#34495e"},
        {100,"Jalahalli Metro","metro",13.0353,77.5442,"#34495e"}
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
