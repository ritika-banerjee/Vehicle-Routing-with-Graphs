#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <time.h>

#define MAX_NODES 100
#define MAX_VEHICLES 10
#define INF INT_MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Structure to represent a time window
typedef struct {
    int earliest;
    int latest;
    int service_time;
} TimeWindow;

// Structure to represent a customer/location
typedef struct {
    int x, y;              // Coordinates
    TimeWindow time_window;
    bool is_served;
    int demand;            // Customer demand
} Customer;

// Structure to represent a vehicle
typedef struct {
    int capacity;
    int current_load;
    int current_time;
    int current_location;
    int *route;
    int route_size;
    int route_capacity;  // Add this to track allocated size
} Vehicle;


// Global variables
Customer customers[MAX_NODES];
int distances[MAX_NODES][MAX_NODES];
Vehicle vehicles[MAX_VEHICLES];
int n_customers, n_vehicles;

// Calculate Euclidean distance between two points
int calculate_distance(Customer a, Customer b) {
    int dx = a.x - b.x;
    int dy = a.y - b.y;
    return (int)(sqrt(dx*dx + dy*dy));
}

// Initialize the distance matrix
void initialize_distances() {
    DEBUG_PRINT("Initializing distance matrix");
    for (int i = 0; i < n_customers; i++) {
        for (int j = 0; j < n_customers; j++) {
            distances[i][j] = calculate_distance(customers[i], customers[j]);
            DEBUG_PRINT("Distance [%d][%d] = %d", i, j, distances[i][j]);
        }
    }
}

// Check if adding a customer to a vehicle's route is feasible
bool is_feasible(Vehicle* vehicle, int customer_id) {
    if (vehicle->current_load + customers[customer_id].demand > vehicle->capacity) {
        DEBUG_PRINT("Customer %d exceeds vehicle capacity", customer_id);
        return false;
    }
    
    int arrival_time = vehicle->current_time + 
        distances[vehicle->current_location][customer_id];
    
    if (arrival_time > customers[customer_id].time_window.latest) {
        DEBUG_PRINT("Customer %d time window violation", customer_id);
        return false;
    }
    
    return true;
}

// Dynamic programming for route optimization
void optimize_route(Vehicle* vehicle) {
    DEBUG_PRINT("Optimizing route for vehicle");
    
    if (vehicle->route_size <= 2) return;  // Nothing to optimize
    
    // Create temporary arrays for optimization
    int* best_route = (int*)malloc(vehicle->route_capacity * sizeof(int));
    int best_cost = INF;
    
    if (!best_route) {
        fprintf(stderr, "Memory allocation failed in optimize_route\n");
        return;
    }
    
    // Copy current route as best known solution
    memcpy(best_route, vehicle->route, vehicle->route_size * sizeof(int));
    
    // Simple 2-opt optimization
    bool improved;
    do {
        improved = false;
        for (int i = 1; i < vehicle->route_size - 2; i++) {
            for (int j = i + 1; j < vehicle->route_size - 1; j++) {
                // Try reversing the route between i and j
                int current_cost = distances[vehicle->route[i-1]][vehicle->route[i]] +
                                 distances[vehicle->route[j]][vehicle->route[j+1]];
                int new_cost = distances[vehicle->route[i-1]][vehicle->route[j]] +
                              distances[vehicle->route[i]][vehicle->route[j+1]];
                
                if (new_cost < current_cost) {
                    // Reverse the segment if it improves the solution
                    for (int k = 0; k < (j - i + 1) / 2; k++) {
                        int temp = vehicle->route[i + k];
                        vehicle->route[i + k] = vehicle->route[j - k];
                        vehicle->route[j - k] = temp;
                    }
                    improved = true;
                }
            }
        }
    } while (improved);
    
    // Verify time windows are still satisfied
    bool feasible = true;
    int current_time = 0;
    
    for (int i = 1; i < vehicle->route_size; i++) {
        int customer = vehicle->route[i];
        current_time += distances[vehicle->route[i-1]][customer];
        
        if (current_time > customers[customer].time_window.latest) {
            feasible = false;
            break;
        }
        
        if (current_time < customers[customer].time_window.earliest) {
            current_time = customers[customer].time_window.earliest;
        }
        
        current_time += customers[customer].time_window.service_time;
    }
    
    // Restore best route if new route is infeasible
    if (!feasible) {
        memcpy(vehicle->route, best_route, vehicle->route_size * sizeof(int));
    }
    
    free(best_route);
    DEBUG_PRINT("Route optimization completed");
}

// Construct initial solution
void construct_initial_solution() {
    DEBUG_PRINT("Constructing initial solution");
    
    // Initialize vehicles
    for (int i = 0; i < n_vehicles; i++) {
        vehicles[i].capacity = 100;
        vehicles[i].current_load = 0;
        vehicles[i].current_time = 0;
        vehicles[i].current_location = 0;
        vehicles[i].route_capacity = MAX_NODES;
        vehicles[i].route = (int*)malloc(MAX_NODES * sizeof(int));
        if (!vehicles[i].route) {
            fprintf(stderr, "Memory allocation failed for vehicle route\n");
            exit(1);
        }
        vehicles[i].route_size = 1;
        vehicles[i].route[0] = 0;  // Start at depot
        
        DEBUG_PRINT("Initialized vehicle %d", i);
    }
    
    // Modified customer assignment logic
    bool customers_remaining = true;
    int current_vehicle = 0;
    
    while (customers_remaining) {
        customers_remaining = false;
        bool vehicle_assigned = false;
        
        for (int i = 1; i < n_customers; i++) {
            if (!customers[i].is_served) {
                customers_remaining = true;
                Vehicle* vehicle = &vehicles[current_vehicle];
                
                // Calculate actual arrival time considering current route
                int arrival_time = vehicle->current_time;
                if (vehicle->route_size > 0) {
                    arrival_time += distances[vehicle->current_location][i];
                }
                
                // Check feasibility with proper time window handling
                if (vehicle->current_load + customers[i].demand <= vehicle->capacity &&
                    arrival_time <= customers[i].time_window.latest) {
                    
                    // Adjust arrival time if arriving before earliest time window
                    if (arrival_time < customers[i].time_window.earliest) {
                        arrival_time = customers[i].time_window.earliest;
                    }
                    
                    // Add customer to route
                    if (vehicle->route_size < vehicle->route_capacity) {
                        vehicle->route[vehicle->route_size++] = i;
                        vehicle->current_load += customers[i].demand;
                        vehicle->current_time = arrival_time + customers[i].time_window.service_time;
                        vehicle->current_location = i;
                        customers[i].is_served = true;
                        vehicle_assigned = true;
                        DEBUG_PRINT("Assigned customer %d to vehicle %d", i, current_vehicle);
                    }
                } else {
                    DEBUG_PRINT("Customer %d not feasible for vehicle %d", i, current_vehicle);
                }
            }
        }
        
        // Move to next vehicle if current one couldn't serve any more customers
        if (!vehicle_assigned && customers_remaining) {
            current_vehicle = (current_vehicle + 1) % n_vehicles;
            if (current_vehicle == 0) {
                DEBUG_PRINT("Warning: Some customers could not be served");
                break;
            }
        }
    }
    
    // Add return to depot for all used vehicles
    for (int i = 0; i < n_vehicles; i++) {
        if (vehicles[i].route_size > 1) {
            if (vehicles[i].route_size < vehicles[i].route_capacity) {
                vehicles[i].route[vehicles[i].route_size++] = 0;
                DEBUG_PRINT("Added depot return for vehicle %d", i);
            }
        }
    }
}

// Solve VRPTW
void solve_vrptw() {
    printf("\nStarting VRPTW solution...\n");
    
    initialize_distances();
    construct_initial_solution();
    
    printf("\nOptimizing routes...\n");
    for (int i = 0; i < n_vehicles; i++) {
        optimize_route(&vehicles[i]);
    }
    
    printf("\nVRPTW Solution:\n");
    for (int i = 0; i < n_vehicles; i++) {
        printf("Vehicle %d route: ", i + 1);
        for (int j = 0; j < vehicles[i].route_size; j++) {
            printf("%d ", vehicles[i].route[j]);
        }
        printf("\n");
    }
}

// Print route details
void print_route_details(Vehicle* vehicle, int vehicle_id) {
    printf("\n========= Vehicle %d Details =========\n", vehicle_id + 1);
    printf("Capacity: %d/%d\n", vehicle->current_load, vehicle->capacity);
    printf("Total Time: %d minutes\n", vehicle->current_time);
    printf("Route: Depot");
    
    int total_distance = 0;
    int current_time = 0;
    
    for (int i = 1; i < vehicle->route_size; i++) {
        int prev = vehicle->route[i-1];
        int curr = vehicle->route[i];
        total_distance += distances[prev][curr];
        
        current_time += distances[prev][curr];
        if (curr != 0) {  // Don't print details for depot
            printf("\n→ Customer %d:", curr);
            printf("\n  Location: (%d, %d)", customers[curr].x, customers[curr].y);
            printf("\n  Arrival Time: %d", current_time);
            printf("\n  Time Window: [%d, %d]", 
                   customers[curr].time_window.earliest,
                   customers[curr].time_window.latest);
            printf("\n  Service Time: %d", customers[curr].time_window.service_time);
            printf("\n  Demand: %d", customers[curr].demand);
            
            current_time += customers[curr].time_window.service_time;
        } else {
            printf("\n→ Return to Depot");
        }
    }
    
    printf("\nTotal Distance: %d units\n", total_distance);
    printf("=====================================\n");
}

// Visualize routes
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

const int GRID_SIZE = 50;

void convert_coord(int x, int y, int min_x, int min_y, double scale, int offset_x, int offset_y, int* grid_x, int* grid_y) {
    *grid_x = offset_x + (int)((x - min_x) * scale);
    *grid_y = offset_y + (int)((y - min_y) * scale);

    // Ensure coordinates are within grid bounds
    *grid_x = MAX(0, MIN(*grid_x, GRID_SIZE - 1));
    *grid_y = MAX(0, MIN(*grid_y, GRID_SIZE - 1));
}

void visualize_routes(int cust, int veh) {
    char grid[GRID_SIZE][GRID_SIZE];
    int n_customers = cust;
    int n_vehicles = veh;
    int min_x = 0, max_x = 0, min_y = 0, max_y = 0;
    
    // Initialize grid with spaces
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = ' ';
        }
    }

    // Find the bounds of customer coordinates
    for (int i = 0; i < n_customers; i++) {
        min_x = MIN(min_x, customers[i].x);
        max_x = MAX(max_x, customers[i].x);
        min_y = MIN(min_y, customers[i].y);
        max_y = MAX(max_y, customers[i].y);
    }

    // Add some padding
    min_x -= 1;
    min_y -= 1;
    max_x += 1;
    max_y += 1;

    // Calculate scaling factors to fit the grid
    double scale_x = (GRID_SIZE - 4) / (double)(max_x - min_x);
    double scale_y = (GRID_SIZE - 4) / (double)(max_y - min_y);
    double scale = MIN(scale_x, scale_y);

    // Calculate centering offsets
    int offset_x = (GRID_SIZE - (int)((max_x - min_x) * scale)) / 2;
    int offset_y = (GRID_SIZE - (int)((max_y - min_y) * scale)) / 2;

    // Mark depot
    int depot_x, depot_y;
    convert_coord(customers[0].x, customers[0].y, min_x, min_y, scale, offset_x, offset_y, &depot_x, &depot_y);
    grid[depot_y][depot_x] = 'D';

    // Draw routes for each vehicle
    for (int v = 0; v < n_vehicles; v++) {
        Vehicle* vehicle = &vehicles[v];
        if (vehicle->route_size <= 1) continue;

        // Draw path for this vehicle
        for (int i = 0; i < vehicle->route_size - 1; i++) {
            int curr_customer = vehicle->route[i];
            int next_customer = vehicle->route[i + 1];

            int x1, y1, x2, y2;
            convert_coord(customers[curr_customer].x, customers[curr_customer].y, min_x, min_y, scale, offset_x, offset_y, &x1, &y1);
            convert_coord(customers[next_customer].x, customers[next_customer].y, min_x, min_y, scale, offset_x, offset_y, &x2, &y2);

            // Mark customer locations (except depot)
            if (curr_customer != 0) {
                grid[y1][x1] = 'C';
            }
            if (next_customer != 0) {
                grid[y2][x2] = 'C';
            }

            // Draw path using Bresenham's line algorithm
            int dx = abs(x2 - x1);
            int dy = abs(y2 - y1);
            int sx = x1 < x2 ? 1 : -1;
            int sy = y1 < y2 ? 1 : -1;
            int err = (dx > dy ? dx : -dy) / 2;
            int e2;

            int x = x1, y = y1;
            while (true) {
                if (x == x2 && y == y2) break;

                if (grid[y][x] == ' ') {
                    grid[y][x] = '.';
                }

                e2 = err;
                if (e2 > -dx) {
                    err -= dy;
                    x += sx;
                }
                if (e2 < dy) {
                    err += dx;
                    y += sy;
                }
            }
        }
    }

    // Print visualization with a border
    printf("\n====== Route Visualization ======\n");
    printf("D: Depot, C: Customer, .: Path\n\n");

    // Print top border
    printf("+");
    for (int i = 0; i < GRID_SIZE; i++) printf("-");
    printf("+\n");

    // Print grid with side borders
    for (int i = 0; i < GRID_SIZE; i++) {
        printf("|");
        for (int j = 0; j < GRID_SIZE; j++) {
            printf("%c", grid[i][j]);
        }
        printf("|\n");
    }

    // Print bottom border
    printf("+");
    for (int i = 0; i < GRID_SIZE; i++) printf("-");
    printf("+\n");

    printf("==============================\n");
}


// Print statistics
void print_statistics() {
    printf("\n====== Solution Statistics ======\n");
    
    int total_distance = 0;
    int total_load = 0;
    int max_time = 0;
    
    for (int i = 0; i < n_vehicles; i++) {
        Vehicle* vehicle = &vehicles[i];
        int vehicle_distance = 0;
        
        for (int j = 1; j < vehicle->route_size; j++) {
            int prev = vehicle->route[j-1];
            int curr = vehicle->route[j];
            vehicle_distance += distances[prev][curr];
        }
        
        total_distance += vehicle_distance;
        total_load += vehicle->current_load;
        max_time = MAX(max_time, vehicle->current_time);
        
        printf("Vehicle %d:\n", i + 1);
        printf("  Distance: %d units\n", vehicle_distance);
        printf("  Load: %d/%d\n", vehicle->current_load, vehicle->capacity);
        printf("  Time: %d minutes\n", vehicle->current_time);
    }
    
    printf("\nOverall Statistics:\n");
    printf("Total Distance: %d units\n", total_distance);
    printf("Total Load: %d units\n", total_load);
    printf("Maximum Route Time: %d minutes\n", max_time);
    printf("==============================\n");
}

// Interactive menu
void interactive_menu() {
    char buffer[256];
    
    while (1) {
        printf("\nVRPTW Interactive Menu:\n");
        printf("1. Show All Route Details\n");
        printf("2. Show Route Visualization\n");
        printf("3. Show Solution Statistics\n");
        printf("4. Modify Customer Data\n");
printf("5. Re-optimize Routes\n");
printf("6. Exit\n");
printf("\nEnter your choice: ");

if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
    printf("Error reading input\n");
    continue;
}

int choice = atoi(buffer);

switch (choice) {
    case 1:
        printf("\n=== Detailed Route Information ===\n");
        for (int i = 0; i < n_vehicles; i++) {
            print_route_details(&vehicles[i], i);
        }
        break;
        
    case 2:
        visualize_routes(n_customers,n_vehicles);
        break;
        
    case 3:
        print_statistics();
        break;
        
    case 4: {
        printf("\nEnter customer ID to modify (1-%d): ", n_customers - 1);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("Error reading input\n");
            break;
        }
        int customer_id = atoi(buffer);
        
        if (customer_id <= 0 || customer_id >= n_customers) {
            printf("Invalid customer ID\n");
            break;
        }
        
        printf("Current customer data:\n");
        printf("Location: (%d, %d)\n", customers[customer_id].x, customers[customer_id].y);
        printf("Time Window: [%d, %d]\n", 
               customers[customer_id].time_window.earliest,
               customers[customer_id].time_window.latest);
        printf("Service Time: %d\n", customers[customer_id].time_window.service_time);
        printf("Demand: %d\n", customers[customer_id].demand);
        
        printf("\nEnter new x coordinate: ");
        fgets(buffer, sizeof(buffer), stdin);
        customers[customer_id].x = atoi(buffer);
        
        printf("Enter new y coordinate: ");
        fgets(buffer, sizeof(buffer), stdin);
        customers[customer_id].y = atoi(buffer);
        
        printf("Enter new earliest time: ");
        fgets(buffer, sizeof(buffer), stdin);
        customers[customer_id].time_window.earliest = atoi(buffer);
        
        printf("Enter new latest time: ");
        fgets(buffer, sizeof(buffer), stdin);
        customers[customer_id].time_window.latest = atoi(buffer);
        
        printf("Enter new service time: ");
        fgets(buffer, sizeof(buffer), stdin);
        customers[customer_id].time_window.service_time = atoi(buffer);
        
        printf("Enter new demand: ");
        fgets(buffer, sizeof(buffer), stdin);
        customers[customer_id].demand = atoi(buffer);
        
        // Reinitialize distances after location change
        initialize_distances();
        printf("Customer data updated successfully\n");
        break;
    }
        
    case 5:
        // Reset all customer served flags
        for (int i = 1; i < n_customers; i++) {
            customers[i].is_served = false;
        }
        
        // Free existing routes
        for (int i = 0; i < n_vehicles; i++) {
            free(vehicles[i].route);
        }
        
        // Reconstruct and optimize solution
        construct_initial_solution();
        for (int i = 0; i < n_vehicles; i++) {
            optimize_route(&vehicles[i]);
        }
        printf("Routes have been re-optimized\n");
        break;
        
    case 6:
        printf("Exiting program...\n");
        // Free allocated memory
        for (int i = 0; i < n_vehicles; i++) {
            free(vehicles[i].route);
        }
        return;
        
    default:
        printf("Invalid choice. Please try again.\n");
        break;
}
}
}

// Main function with example problem instance
int main() {
    srand(time(NULL));  // Initialize random seed
    
    // Set up example problem
    n_customers = 10;  // Including depot
    n_vehicles = 3;
    
    // Initialize depot (customer 0)
    customers[0].x = 0;
    customers[0].y = 0;
    customers[0].time_window.earliest = 0;
    customers[0].time_window.latest = INF;
    customers[0].time_window.service_time = 0;
    customers[0].demand = 0;
    customers[0].is_served = true;
    
    // Generate random customers
    for (int i = 1; i < n_customers; i++) {
        customers[i].x = rand() % 20 - 10;  // Random coordinates between -10 and 10
        customers[i].y = rand() % 20 - 10;
        customers[i].time_window.earliest = rand() % 100;  // Random time windows
        customers[i].time_window.latest = customers[i].time_window.earliest + 50 + rand() % 100;
        customers[i].time_window.service_time = 10 + rand() % 20;  // Service time between 10-30
        customers[i].demand = 5 + rand() % 20;  // Demand between 5-25
        customers[i].is_served = false;
    }
    
    printf("VRPTW Solver\n");
    printf("Customers: %d\n", n_customers - 1);
    printf("Vehicles: %d\n", n_vehicles);
    
    // Solve the problem
    solve_vrptw();
    
    // Enter interactive menu
    interactive_menu();
    
    return 0;
}