#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <list>
#include <queue>
#include <climits>
#include <utility>
#include <unordered_map>

using namespace std;

// -------------------- STRUCTS --------------------
struct Vehicle {
    string vehicleID;
    string type;
    int entryTime;
};

struct ParkingSlot {
    int slotID;
    string size;
    bool isOccupied;
    string currentVehicleID;
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
        
        if (slot.slotID < node->slot.slotID) {
            node->left = insert(node->left, slot);
        } else if (slot.slotID > node->slot.slotID) {
            node->right = insert(node->right, slot);
        }
        return node;
    }

    void inorder(SlotNode* node) {
        if (!node) return;
        inorder(node->left);
        cout << "Slot " << node->slot.slotID << " | Size: " << node->slot.size 
            << " | Occupied: " << (node->slot.isOccupied ? "Yes (" + node->slot.currentVehicleID + ")" : "No") << endl;
        inorder(node->right);
    }
    
    SlotNode* findSlot(SlotNode* node, int slotID) {
        if (!node || node->slot.slotID == slotID) return node;
        if (slotID < node->slot.slotID) return findSlot(node->left, slotID);
        return findSlot(node->right, slotID);
    }
    
    SlotNode* searchAvailable(SlotNode* node, string type) {
        if (!node) return nullptr;

        SlotNode* leftResult = searchAvailable(node->left, type);
        if (leftResult) return leftResult;

        if (!node->slot.isOccupied && node->slot.size == type) {
            return node;
        }

        return searchAvailable(node->right, type);
    }
};

// -------------------- Graph for shortest path --------------------
class FloorGraph {
    int V;
    vector<vector<int>> adj;
public:
    FloorGraph(int V) : V(V) { adj.resize(V); } 

    void addEdge(int u, int v) { adj[u].push_back(v); adj[v].push_back(u); }

    int shortestPath(int src, int dest) {
        if (src < 0 || src >= V || dest < 0 || dest >= V) return -1;
        
        vector<int> dist(V, INT_MAX);
        queue<int> q;
        q.push(src);
        dist[src] = 0;

        while(!q.empty()) {
            int u = q.front(); q.pop();
            if (u == dest) return dist[dest];
            
            for(int v: adj[u]) {
                if(dist[v] == INT_MAX) {
                    dist[v] = dist[u] + 1;
                    q.push(v);
                }
            }
        }
        return -1;
    }
};

// -------------------- Hash Table for Vehicle Lookup --------------------
// Store vehicle ID -> vector of allocated slot IDs
unordered_map<string, vector<int>> vehicleMap;

// -------------------- Allocation Functions --------------------
bool allocateSingleSlot(vector<ParkingSlot>& slots, const string& vehicleType, const string& vehicleID, vector<int>& res) {
    // Find first available slot matching the type
    for (auto& slot : slots) {
        if (!slot.isOccupied && slot.size == vehicleType) {
            slot.isOccupied = true;
            slot.currentVehicleID = vehicleID;
            res.push_back(slot.slotID);
            return true;
        }
    }
    return false;
}

bool allocateContiguousSlots(vector<ParkingSlot>& slots, int needed, const string& vehicleID, vector<int>& res) {
    int n = slots.size();
    for (int i = 0; i <= n - needed; ++i) {
        bool available = true;
        vector<int> currentAllocation;
        
        for (int j = 0; j < needed; ++j) {
            if (slots[i+j].isOccupied) { 
                available = false;
                break;
            }
            currentAllocation.push_back(slots[i+j].slotID);
        }

        if (available) {
            res = currentAllocation;
            // Update all allocated slots
            for (int j = 0; j < needed; ++j) {
                slots[i+j].isOccupied = true;
                slots[i+j].currentVehicleID = vehicleID;
            }
            return true;
        }
    }
    return false;
}

// -------------------- Dynamic Programming for scheduling --------------------
int maxVehicles(vector<int>& entry, vector<int>& exit) {
    int n = entry.size();
    if (n == 0) return 0;
    
    vector<pair<int,int>> times(n);
    for(int i=0; i<n; i++) times[i] = {exit[i], entry[i]};
    sort(times.begin(), times.end());

    vector<int> dp(n, 1);
    int maxCount = (n > 0) ? 1 : 0;
    
    for(int i = 1; i < n; i++){
        for(int j = 0; j < i; j++){
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
    vector<ParkingSlot> slots = {
        {1,"small",false, ""}, {2,"medium",false, ""}, {3,"large",false, ""},
        {4,"medium",false, ""}, {5,"small",false, ""}, {6,"large",false, ""}
    };
    
    FloorGraph g(slots.size());
    for(size_t i = 0; i < slots.size() - 1; i++) g.addEdge(i, i+1);
    g.addEdge(1, 4);

    int choice;
    while(true) {
        cout << "\nParking System Menu:\n1. Show Slots\n2. Park Vehicle\n3. Remove Vehicle\n4. Shortest Path (between slot indices)\n5. Max Scheduling (DP)\n6. Exit\nChoice: ";
        cin >> choice;

        if(choice == 1){
            SlotBST bst;
            for(auto s: slots) bst.root = bst.insert(bst.root, s);
            bst.inorder(bst.root);
            
        } else if(choice == 2){
            string id, type;
            int needed = 1;
            cout << "Vehicle ID: "; cin >> id;
            cout << "Type (small/medium/large/bus): "; cin >> type;
            
            if(type == "bus") {
                needed = 3;
                cout << "Bus requires " << needed << " contiguous slots.\n";
            } else if (type == "small" || type == "medium" || type == "large") {
                needed = 1;
            } else {
                cout << "Invalid vehicle type.\n";
                continue;
            }
            
            if (vehicleMap.count(id)) {
                cout << "Vehicle already parked.\n";
                continue;
            }

            vector<int> allocation;
            bool success = false;
            
            if (needed == 1) {
                success = allocateSingleSlot(slots, type, id, allocation);
            } else {
                success = allocateContiguousSlots(slots, needed, id, allocation);
            }
            
            if(success){
                vehicleMap[id] = allocation;
                cout << "Allocated slots: ";
                for(int s: allocation) cout << s << " ";
                cout << endl;
            } else {
                cout << "No slots available for " << type << ".\n";
            }

        } else if(choice == 3){
            string id;
            cout << "Vehicle ID: ";
            cin >> id;
            
            if(vehicleMap.find(id) != vehicleMap.end()){
                vector<int> allocatedSlots = vehicleMap[id];
                
                // Free all slots occupied by this vehicle
                for(int slotID : allocatedSlots) {
                    for(auto &s: slots) {
                        if(s.slotID == slotID && s.currentVehicleID == id) {
                            s.isOccupied = false;
                            s.currentVehicleID = "";
                        }
                    }
                }
                
                vehicleMap.erase(id);
                cout << "Vehicle " << id << " removed from slots: ";
                for(int s : allocatedSlots) cout << s << " ";
                cout << endl;
            } else {
                cout << "Vehicle not found.\n";
            }

        } else if(choice == 4){
            int src, dest;
            cout << "Source slot ID (1 to " << slots.size() << "): "; cin >> src;
            cout << "Destination slot ID (1 to " << slots.size() << "): "; cin >> dest;
            
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