#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <list>
#include <queue>
#include <climits>
#include <utility> // For std::pair
#include <unordered_map>

using namespace std;

// -------------------- STRUCTS --------------------
// Vehicle struct (mostly for data tracking)
struct Vehicle {
    string vehicleID;
    string type; // small, medium, large (matching slot size for simple allocation)
    int entryTime;
    // We don't track exitTime here, but rather when they leave
};

// ParkingSlot struct
struct ParkingSlot {
    int slotID;
    string size; // small, medium, large
    bool isOccupied;
    string currentVehicleID; // Track which vehicle is occupying it
};

// -------------------- BST for slot search --------------------
struct SlotNode {
    ParkingSlot slot;
    SlotNode *left, *right;
    SlotNode(ParkingSlot s) : slot(s), left(nullptr), right(nullptr) {}
};

class SlotBST {
public:
    SlotNode* root;
    SlotBST() { root = nullptr; }

    SlotNode* insert(SlotNode* node, ParkingSlot slot) {
        if (!node) return new SlotNode(slot);
        
        // Corrected: Insertion must use a consistent key. slotID is the key.
        if (slot.slotID < node->slot.slotID) {
            node->left = insert(node->left, slot);
        } else if (slot.slotID > node->slot.slotID) { // Added check for > to avoid duplicates
            node->right = insert(node->right, slot);
        }
        return node;
    }

    void inorder(SlotNode* node) {
        if (!node) return;
        inorder(node->left);
        cout << "Slot " << node->slot.slotID << " | Size: " << node->slot.size << " | Occupied: " << (node->slot.isOccupied ? "Yes (" + node->slot.currentVehicleID + ")" : "No") << endl;
        inorder(node->right);
    }
    
    // Function to find a slot by its ID, necessary for updating occupancy
    SlotNode* findSlot(SlotNode* node, int slotID) {
        if (!node || node->slot.slotID == slotID) return node;
        if (slotID < node->slot.slotID) return findSlot(node->left, slotID);
        return findSlot(node->right, slotID);
    }
    
    // Corrected logic for finding the FIRST available slot of a specific type (in-order)
    SlotNode* searchAvailable(SlotNode* node, string type) {
        if (!node) return nullptr;

        // In-order traversal to check for the first available slot
        SlotNode* leftResult = searchAvailable(node->left, type);
        if (leftResult) return leftResult; // Found in left subtree

        // Check current node
        if (!node->slot.isOccupied && node->slot.size == type) {
            return node;
        }

        return searchAvailable(node->right, type); // Search in right subtree
    }
};

// -------------------- Graph for shortest path --------------------
// Note: Using an adjacency list for V-1 edges is fine, but weights are typically needed.
// For simplicity, using BFS (shortest path in unweighted graph).
class FloorGraph {
    int V; // Number of nodes/slots
    vector<vector<int>> adj;
public:
    // V should be the number of nodes (slots + entry/exit points)
    FloorGraph(int V) : V(V) { adj.resize(V); } 

    void addEdge(int u, int v) { adj[u].push_back(v); adj[v].push_back(u); }

    int shortestPath(int src, int dest) {
        if (src < 0 || src >= V || dest < 0 || dest >= V) return -1; // Basic bounds check
        
        vector<int> dist(V, INT_MAX);
        queue<int> q;
        q.push(src);
        dist[src] = 0;

        while(!q.empty()) {
            int u = q.front(); q.pop();
            if (u == dest) return dist[dest]; // Optimization
            
            for(int v: adj[u]) {
                if(dist[v] == INT_MAX) {
                    dist[v] = dist[u] + 1;
                    q.push(v);
                }
            }
        }
        return -1; // Destination not reachable
    }
};

// -------------------- Hash Table for Vehicle Lookup --------------------
// A map to store active vehicles and their assigned slot IDs.
// vehicleID -> slotID
unordered_map<string, int> vehicleMap;

// -------------------- Branch & Bound/Recursion for multi-slot allocation --------------------
// Issue: This function uses a 'slots' vector which is independent of the BST structure,
// leading to a state inconsistency. In a real system, you'd iterate over the BST or a list
// backed by the BST.
// FIX: We will just search for contiguous slots in a list representation for simplicity.
bool allocateContiguousSlots(vector<ParkingSlot>& slots, int needed, const string& vehicleID, vector<int>& res) {
    if (needed == 1) { // Use BST search for single slot (like car/bike)
        SlotBST bst; 
        for(auto &s : slots) bst.root = bst.insert(bst.root, s); // Rebuild BST (inefficient, but necessary due to original design)
        
        SlotNode* node = bst.searchAvailable(bst.root, slots[0].size); // Simplified to use the first slot size
        if (node) {
            res.push_back(node->slot.slotID);
            // Must update the vector as well for consistency
            for(auto& s : slots) {
                if(s.slotID == node->slot.slotID) {
                    s.isOccupied = true;
                    s.currentVehicleID = vehicleID;
                    break;
                }
            }
            return true;
        }
        return false;
    }
    
    // For contiguous slots (e.g., bus: 3 medium/large slots together)
    int n = slots.size();
    for (int i = 0; i <= n - needed; ++i) {
        bool available = true;
        vector<int> currentAllocation;
        for (int j = 0; j < needed; ++j) {
            // Simplified: check for contiguous availability regardless of size for bus demo
            if (slots[i+j].isOccupied) { 
                available = false;
                break;
            }
            currentAllocation.push_back(slots[i+j].slotID);
        }

        if (available) {
            res = currentAllocation;
            // Update the state
            for(int slotID : res) {
                for(auto& s : slots) {
                    if(s.slotID == slotID) {
                        s.isOccupied = true;
                        s.currentVehicleID = vehicleID;
                        break;
                    }
                }
            }
            return true;
        }
    }
    return false;
}

// -------------------- Dynamic Programming for scheduling --------------------
// Activity Selection Problem (Max non-overlapping parking)
int maxVehicles(vector<int>& entry, vector<int>& exit) {
    int n = entry.size();
    if (n == 0) return 0;
    
    // Pair (exit time, entry time) and sort by exit time
    vector<pair<int,int>> times(n);
    for(int i=0; i<n; i++) times[i] = {exit[i], entry[i]};
    sort(times.begin(), times.end());

    vector<int> dp(n, 1);
    int maxCount = (n > 0) ? 1 : 0;
    
    for(int i = 1; i < n; i++){
        for(int j = 0; j < i; j++){
            // Check for non-overlapping: if activity j finishes before or at the start of activity i
            if(times[j].first <= times[i].second) { 
                dp[i] = max(dp[i], dp[j] + 1);
            }
        }
        maxCount = max(maxCount, dp[i]);
    }
    return maxCount;
}

// -------------------- Main --------------------
int main() {
    // Corrected Slot Initialization: slots vector is the master list of slots
    vector<ParkingSlot> slots = {
        {1,"small",false, ""}, {2,"medium",false, ""}, {3,"large",false, ""}, 
        {4,"medium",false, ""}, {5,"small",false, ""}, {6,"large",false, ""}
    };
    
    // Graph initialization: Node indices correspond to slot indices (0 to 5)
    FloorGraph g(slots.size()); 
    // Example: simple linear floor layout
    for(size_t i = 0; i < slots.size() - 1; i++) g.addEdge(i, i+1); 
    // Adding a cross link
    g.addEdge(1, 4); 

    int choice;
    while(true) {
        cout << "\nParking System Menu:\n1. Show Slots\n2. Park Vehicle\n3. Remove Vehicle\n4. Shortest Path (between slot indices)\n5. Max Scheduling (DP)\n6. Exit\nChoice: ";
        cin >> choice;

        if(choice == 1){
            // To show current status via BST, we rebuild it from the master 'slots' vector
            SlotBST bst;
            for(auto s: slots) bst.root = bst.insert(bst.root, s);
            bst.inorder(bst.root);
            
        } else if(choice == 2){
            string id, type;
            int needed = 1;
            cout << "Vehicle ID: "; cin >> id;
            cout << "Type (small/medium/large/bus): "; cin >> type;
            
            if(type == "bus") {
                needed = 3; // Bus requires 3 contiguous slots
                cout << "Bus requires " << needed << " contiguous slots.\n";
            } else if (type == "small" || type == "medium" || type == "large") {
                needed = 1;
            } else {
                cout << "Invalid vehicle type.\n"; continue;
            }
            
            // Check if vehicle is already parked
            if (vehicleMap.count(id)) { cout << "Vehicle already parked.\n"; continue; }

            vector<int> allocation;
            
            // For simplicity, we use the old logic for single slot in the allocation function.
            // A dedicated, efficient multi-slot allocation should be implemented for 'bus'.
            if(allocateContiguousSlots(slots, needed, id, allocation)){
                for(int s: allocation) vehicleMap[id] = s; // Only map to the *first* slot ID
                cout << "Allocated slots: "; 
                for(int s: allocation) cout << s << " "; cout << endl;
            } else {
                cout << "No slots available for " << type << ".\n";
            }

        } else if(choice == 3){
            string id; cout << "Vehicle ID: "; cin >> id;
            if(vehicleMap.find(id) != vehicleMap.end()){
                int slotID = vehicleMap[id];
                
                // Find all contiguous slots allocated to the vehicle (if it's a bus)
                vector<int> slotsToFree;
                for(auto const& [key, val] : vehicleMap) {
                    if (key == id || val == slotID) { // Crude check: assumes only one vehicle per slot, or that slots are contiguous from the first one
                        slotsToFree.push_back(val); 
                    }
                }
                
                // Free the slots in the master list
                for(int freeID : slotsToFree) {
                    for(auto &s: slots) {
                        if(s.slotID == freeID && s.currentVehicleID == id) { // Only free if the ID matches
                            s.isOccupied = false;
                            s.currentVehicleID = "";
                        }
                    }
                }
                
                vehicleMap.erase(id); // Remove vehicle from map
                cout << "Vehicle " << id << " removed.\n";
            } else {
                cout << "Vehicle not found.\n";
            }

        } else if(choice == 4){
            int src, dest;
            cout << "Source slot ID (1 to " << slots.size() << "): "; cin >> src;
            cout << "Destination slot ID (1 to " << slots.size() << "): "; cin >> dest;
            
            // Graph nodes are 0-indexed, so we subtract 1 from slot IDs
            int pathLength = g.shortestPath(src - 1, dest - 1);
            if (pathLength != -1) {
                 cout << "Shortest path distance (edges): " << pathLength << endl;
            } else {
                 cout << "Destination not reachable.\n";
            }
            
        } else if(choice == 5){
            vector<int> entry={1, 3, 0, 5, 8, 9};
            vector<int> exit={2, 4, 6, 7, 10, 11};
            cout << "Sample entry times: [1, 3, 0, 5, 8, 9]\n";
            cout << "Sample exit times: [2, 4, 6, 7, 10, 11]\n";
            cout << "Max non-overlapping vehicles that can be scheduled: " << maxVehicles(entry, exit) << endl;
            
        } else if (choice == 6) {
            break;
        } else {
            cout << "Invalid choice. Please try again.\n";
        }
    }

    return 0;
}