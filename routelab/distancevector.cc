#include "distancevector.h"

DistanceVector::DistanceVector(unsigned n, SimulationContext* c, double b, double l) :
    Node(n, c, b, l)
{}

DistanceVector::DistanceVector(const DistanceVector & rhs) :
    Node(rhs)
{
    *this = rhs;
}

DistanceVector & DistanceVector::operator=(const DistanceVector & rhs) {
    Node::operator=(rhs);
    return *this;
}

DistanceVector::~DistanceVector() {}


/** Write the following functions.  They currently have dummy implementations **/
void DistanceVector::LinkHasBeenUpdated(Link* l) {
    cerr << *this << ": Link Update: " << *l << endl;

    int dest = l->GetDest();
    int latency = l->GetLatency();

    routing_table.neighborLinks[dest].cost = latency;
    routing_table.distanceVector[dest].cost = -1;
    routing_table.topo[dest][dest].cost = 0;

    //Need to update the DV, if it was successful we can send it off
    if (UpdateDistanceVector()) {
        cout << "SendToNeighbors (LinkHasBeenUpdated)" << endl;
        SendToNeighbors(new RoutingMessage(GetNumber(), routing_table.distanceVector));
    }

}

void DistanceVector::ProcessIncomingRoutingMessage(RoutingMessage *m) {
    cerr << *this << " got a routing message: " << *m << " (ignored)" << endl;

    //update topo
    routing_table.topo[m->sendingNode] = m->distanceVector;

    map<int, TopoLink>::const_iterator itr;
    for (itr = m->distanceVector.begin(); itr != m->distanceVector.end(); itr++) {
        if (routing_table.distanceVector[itr->first].cost == -1) {

        }
    }


    if (UpdateDistanceVector()) {
        SendToNeighbors(new RoutingMessage(GetNumber(), routing_table.distanceVector));
    }
}

void DistanceVector::TimeOut() {
    cerr << *this << " got a timeout: (ignored)" << endl;
}

Node* DistanceVector::GetNextHop(Node *destination) {
    Node* ret = NULL;
    deque<Node*>::const_iterator neighbor_itr;

    deque<Node*>* neighbors = GetNeighbors();
    int current_neighbor;
    for (neighbor_itr = neighbors->begin(); neighbor_itr != neighbors->end(); neighbor_itr++) {
        current_neighbor = (*neighbor_itr)->GetNumber();
        if (current_neighbor == routing_table.topoMap[destination->GetNumber()]) {
            ret = new Node(*(*neighbor_itr));
        }
    }

    return ret;
}

Table* DistanceVector::GetRoutingTable() {
    return new Table(routing_table);
}

bool DistanceVector::UpdateDistanceVector() {
    bool updated = false;
    map<int, TopoLink>::const_iterator dv_itr;
    map<int, TopoLink>::const_iterator neighbor_itr;



    for (dv_itr = routing_table.distanceVector.begin(); dv_itr != routing_table.distanceVector.end(); dv_itr++) {
        int current_node = dv_itr->first;
        int current_min = MAX_INT;
        int next_hop = -1;

        if (current_node == (int)GetNumber()) {
            routing_table.distanceVector[current_node].cost = 0;
            continue;
        }



        for (neighbor_itr = routing_table.neighborLinks.begin(); neighbor_itr != routing_table.neighborLinks.end();
             neighbor_itr++) {
            int current_neighbor = neighbor_itr->first;

            if (routing_table.neighborLinks[current_neighbor].cost != -1 &&
                    routing_table.topo[current_neighbor][current_node].cost != -1) {

                int alt_path = routing_table.neighborLinks[current_neighbor].cost +
                           routing_table.topo[current_neighbor][current_node].cost;

                if (alt_path < current_min) {
                    current_min = alt_path;
                    next_hop = current_neighbor;
                }
            }
        }

        if (current_min != MAX_INT && current_min != routing_table.distanceVector[current_node].cost) {
            routing_table.distanceVector[current_node].cost = current_min;
            routing_table.topoMap[current_node] = next_hop;
            updated = true;
        }
    }

        return updated;
}

ostream & DistanceVector::Print(ostream &os) const { 
    Node::Print(os);
    return os;
}
