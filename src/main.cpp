#include <bits/stdc++.h>
using namespace std;

// -------------------- STRUCTS --------------------
struct Vehicle {
    string vehicleID;
    string type; // bike, car, bus
    int entryTime;
    int exitTime;
};

struct ParkingSlot {
    int slotID;
    string size; // small, medium, large
    bool isOccupied;
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
        if (slot.slotID < node->slot.slotID) node->left = insert(node->left, slot);
        else node->right = insert(node->right, slot);
        return node;
    }

    void inorder(SlotNode* node) {
        if (!node) return;
        inorder(node->left);
        cout << "Slot " << node->slot.slotID << " | Size: " << node->slot.size
            << " | Occupied: " << (node->slot.isOccupied ? "Yes" : "No") << endl;
        inorder(node->right);
    }

    SlotNode* searchAvailable(SlotNode* node, string type) {
        if (!node) return nullptr;
        SlotNode* leftSearch = searchAvailable(node->left, type);
        if (leftSearch) return leftSearch;
        if (!node->slot.isOccupied && node->slot.size == type) return node;
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
        vector<int> dist(V, INT_MAX);
        queue<int> q;
        q.push(src);
        dist[src] = 0;

        while(!q.empty()) {
            int u = q.front(); q.pop();
            for(int v: adj[u]) {
                if(dist[v] == INT_MAX) {
                    dist[v] = dist[u]+1;
                    q.push(v);
                }
            }
        }
        return dist[dest];
    }
};

// -------------------- Hash Table for Vehicle Lookup --------------------
unordered_map<string, int> vehicleMap; // vehicleID -> slotID

// -------------------- Branch & Bound for multi-slot allocation --------------------
bool allocateSlots(vector<ParkingSlot>& slots, int idx, int needed, vector<int>& res) {
    if(needed == 0) return true;
    if(idx >= slots.size()) return false;

    if(!slots[idx].isOccupied) {
        slots[idx].isOccupied = true;
        res.push_back(slots[idx].slotID);
        if(allocateSlots(slots, idx+1, needed-1, res)) return true;
        slots[idx].isOccupied = false; // backtrack
        res.pop_back();
    }
    return allocateSlots(slots, idx+1, needed, res);
}

// -------------------- Dynamic Programming for scheduling --------------------
int maxVehicles(vector<int>& entry, vector<int>& exit) {
    int n = entry.size();
    vector<pair<int,int>> times(n);
    for(int i=0;i<n;i++) times[i] = {exit[i], entry[i]};
    sort(times.begin(), times.end());

    vector<int> dp(n,1);
    for(int i=1;i<n;i++){
        for(int j=0;j<i;j++){
            if(times[j].first <= times[i].second)
                dp[i] = max(dp[i], dp[j]+1);
        }
    }
    return *max_element(dp.begin(), dp.end());
}

// -------------------- Main --------------------
int main() {
    // Sample slots
    vector<ParkingSlot> slots = {{1,"small",false},{2,"medium",false},{3,"large",false},{4,"medium",false},{5,"small",false}};
    SlotBST bst;
    for(auto s: slots) bst.root = bst.insert(bst.root, s);

    FloorGraph g(slots.size());
    for(int i=0;i<slots.size()-1;i++) g.addEdge(i, i+1); // simple adjacency

    int choice;
    while(true) {
        cout << "\nParking System Menu:\n1. Show Slots\n2. Park Vehicle\n3. Remove Vehicle\n4. Shortest Path\n5. Optimize Schedule\n6. Exit\nChoice: ";
        cin >> choice;

        if(choice==1){
            bst.inorder(bst.root);
        } else if(choice==2){
            string id, type;
            int needed=1;
            cout << "Vehicle ID: "; cin >> id;
            cout << "Type (small/medium/large): "; cin >> type;
            if(type=="bus") {cout << "Slots needed for bus: "; cin >> needed;}
            
            vector<int> allocation;
            if(allocateSlots(slots,0,needed,allocation)){
                for(int s: allocation) vehicleMap[id]=s;
                cout << "Allocated slots: "; for(int s: allocation) cout << s << " "; cout << endl;
            } else cout << "No slots available\n";
        } else if(choice==3){
            string id; cout << "Vehicle ID: "; cin >> id;
            if(vehicleMap.find(id)!=vehicleMap.end()){
                int slotID = vehicleMap[id];
                for(auto &s: slots) if(s.slotID==slotID) s.isOccupied=false;
                vehicleMap.erase(id);
                cout << "Vehicle removed from slot " << slotID << endl;
            } else cout << "Vehicle not found\n";
        } else if(choice==4){
            int src,dest;
            cout << "Source slot: "; cin >> src;
            cout << "Destination slot: "; cin >> dest;
            cout << "Shortest path distance: " << g.shortestPath(src-1,dest-1) << endl;
        } else if(choice==5){
            vector<int> entry={1,3,0,5};
            vector<int> exit={2,4,6,7};
            cout << "Max vehicles that can be parked: " << maxVehicles(entry,exit) << endl;
        } else break;
    }

    return 0;
}
