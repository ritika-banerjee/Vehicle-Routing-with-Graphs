#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define MAX_NODES 20
#define MAX_VEHICLES 5
#define INF 1e9

typedef struct {
    int x, y;            // Coordinates
    int demand;          // Customer demand
    int earliest, latest; // Time window
} Customer;

typedef struct {
    int capacity;
    int load;
    int location;
    int route[MAX_NODES];
    int route_size;
} Vehicle;

Customer customers[MAX_NODES];
Vehicle vehicles[MAX_VEHICLES];
int distances[MAX_NODES][MAX_NODES];
int n_customers, n_vehicles;

// Calculate Euclidean distance
int calculate_distance(Customer a, Customer b) {
    return (int)(sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2)));
}

// Initialize distance matrix
void initialize_distances() {
    for (int i = 0; i < n_customers; i++) {
        for (int j = 0; j < n_customers; j++) {
            distances[i][j] = calculate_distance(customers[i], customers[j]);
        }
    }
}

// Check if a customer can be added to a vehicle's route
bool is_feasible(Vehicle* vehicle, int customer_id) {
    if (vehicle->load + customers[customer_id].demand > vehicle->capacity) return false;
    return true;
}

// Construct an initial solution by assigning customers to vehicles
void construct_initial_solution() {
    for (int v = 0; v < n_vehicles; v++) {
        vehicles[v].capacity = 50;
        vehicles[v].load = 0;
        vehicles[v].location = 0;
        vehicles[v].route_size = 1;
        vehicles[v].route[0] = 0;  // Start at depot
    }

    for (int i = 1; i < n_customers; i++) {
        bool assigned = false;
        for (int v = 0; v < n_vehicles && !assigned; v++) {
            Vehicle* vehicle = &vehicles[v];
            if (is_feasible(vehicle, i)) {
                vehicle->route[vehicle->route_size++] = i;
                vehicle->load += customers[i].demand;
                assigned = true;
            }
        }
    }

    for (int v = 0; v < n_vehicles; v++) {
        vehicles[v].route[vehicles[v].route_size++] = 0;  // Return to depot
    }
}

// Display route details
void display_routes() {
    for (int v = 0; v < n_vehicles; v++) {
        printf("Vehicle %d route: ", v + 1);
        for (int i = 0; i < vehicles[v].route_size; i++) {
            printf("%d ", vehicles[v].route[i]);
        }
        printf("\n");
    }
}

// Main function
int main() {
    n_customers = 6;  // Including depot
    n_vehicles = 2;

    // Initialize depot
    customers[0] = (Customer) {0, 0, 0, 0, INF};

    // Initialize some customers
    customers[1] = (Customer) {10, 10, 10, 0, INF};
    customers[2] = (Customer) {15, 15, 15, 0, INF};
    customers[3] = (Customer) {20, 5, 20, 0, INF};
    customers[4] = (Customer) {5, 20, 10, 0, INF};
    customers[5] = (Customer) {15, 5, 5, 0, INF};

    initialize_distances();
    construct_initial_solution();
    display_routes();

    return 0;
}
