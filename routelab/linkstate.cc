#include <climits>
#include <map>
#include <queue>
#include <set>
#include <utility>
#include <vector>

#include "linkstate.h"

struct PQEntry {
    int node;
    double latency;

    PQEntry(int n, int l) : node(n), latency(l) {}
};

struct Comp {
    bool operator() (const PQEntry& x, const PQEntry& y) const {
        return x.latency > y.latency;
    }
};

LinkState::LinkState(unsigned n, SimulationContext* c, double b, double l) :
    Node(n, c, b, l), maxNode(number)
{}

LinkState::LinkState(const LinkState & rhs) :
    Node(rhs)
{
    *this = rhs;
}

LinkState & LinkState::operator=(const LinkState & rhs) {
    Node::operator=(rhs);
    return *this;
}

LinkState::~LinkState() {}


/** Write the following functions.  They currently have dummy implementations **/
void LinkState::LinkHasBeenUpdated(Link* l) {
    cerr << *this << ": Link Update: " << *l << endl;

    double latency = (*l).GetLatency();
    int dest = (*l).GetDest();

    maxNode = max(maxNode, dest);

    //cerr << "Link data loaded" << endl;

    if (UpdateGraph(number, dest, latency)) {
        Dijkstra();

        pair<int, int> p (number, dest);

        int sequence;
        map<pair<int, int>, int>::iterator it = current.find(p);
        if (it == current.end()) {
            sequence = 0;
            pair<pair<int, int>, int> p2 (p, sequence);
            current.insert(p2);
        } else {
            sequence = it->second;
            it->second = sequence + 1;
        }

        SendToNeighbors(new RoutingMessage(p, sequence, *l));

    }
}

void LinkState::ProcessIncomingRoutingMessage(RoutingMessage *m) {
    cerr << *this << " got a routing message: " << *m << endl;

    pair<int,int> originator = (*m).originator;
    int sequence = (*m).sequence;
    map<pair<int,int>, int>::iterator it = seen.find(originator);
    if (originator.first == number)
        return;
    else if (it == seen.end()) {
        pair<pair<int, int>, int> p (originator, sequence);
        seen.insert(p);
    } else if (sequence <= it->second)
        return;
    it->second = sequence;

    Link& l = (*m).link;
    double latency = l.GetLatency();
    int src = l.GetSrc();
    int dest = l.GetDest();
    maxNode = max(maxNode, src);
    maxNode = max(maxNode, dest);

    if (UpdateGraph(src, dest, latency)) {
        Dijkstra();

        SendToNeighbors(m);
    }
}

void LinkState::TimeOut() {
    cerr << *this << " got a timeout: (ignored)" << endl;
}

Node* LinkState::GetNextHop(Node *destination) {
    int dest = destination->GetNumber();
    int next = routing_table.getNextHop(dest);

    if (dest == next)
        return destination;

    deque<Node*> *nodes = GetNeighbors();
    deque<Node*>::iterator it;
    for (it = nodes->begin(); it != nodes->end(); it++)
        if ((*it)->GetNumber() == next)
            return *it;
    return NULL;
}

Table* LinkState::GetRoutingTable() {
    return &routing_table;
}

ostream & LinkState::Print(ostream &os) const {
    Node::Print(os);
    return os;
}

// Returns 1 if changed
bool LinkState::UpdateGraph(int src, int dest, double latency) {
    map<int, map<int, double> >::iterator it = graph.find(src);
    if (it != graph.end()) { // If a map exists for this source
        map<int, double> &links = it->second;

        map<int, double>::iterator i = links.find(dest);
        if (i != links.end()) { // If a link exists for this pair
            double old = i->second;
            i->second = latency;
            //cerr << "(" << src << ", " << dest << "): " << latency << endl;
            //cerr << old << " vs. " << latency << endl;
            if (old == latency)
                return 0;
            else
                return 1;
        } else { // If no link exists for this pair
            pair<int, double> newPair(dest, latency);
            links.insert(newPair);
            return 1;
        }
    } else { // If no map exists for this source
        pair<int, map<int, double> > srcPair;
        pair<int, double> destPair(dest, latency);
        srcPair.first = src;
        srcPair.second.insert(destPair);
        graph.insert(srcPair);
        return 1;
    }
}

void LinkState::Dijkstra() {
    map<int, map<int, double> >::iterator i;
    map<int, double>::iterator it;
    priority_queue<PQEntry, vector<PQEntry>, Comp> pq;
    set<int> neighbors;
    int size = max(maxNode, graph.rbegin()->first) + 1;
    bool inc[size];
    int via[size];
    int next[size];
    double cost[size];

    // Initialize arrays
    for (int i = 0; i < size; i++) {
        if (i == number) {
            inc[i] = 1;
            via[i] = i;
            cost[i] = 0;
        } else {
            inc[i] = 0;
            via[i] = UINT_MAX;
            cost[i] = INT_MAX;
        }
    }

    // Initialize with our edges
    i = graph.find(number);
    if (i != graph.end()) {
        map<int, double> &links = i->second;
        for (it = links.begin(); it != links.end(); it++) {
            neighbors.insert(it->first);
            via[it->first] = number;
            cost[it->first] = it->second;
            pq.push(PQEntry(it->first, it->second));
        }
    }

    // Dijkstra's algorithm
    while (!pq.empty()) {
        const PQEntry& e = pq.top();
        int node = e.node;
        double latency = e.latency;
        pq.pop();

        if (inc[node] == 1) // If shortest path already found
            continue;

        inc[node] = 1;
        i = graph.find(node);
        if (i != graph.end()) {
            map<int, double> &links = i->second;
            for (it = links.begin(); it != links.end(); it++) {
                double newCost = cost[node] + it->second;
                if (newCost < cost[it->first]) {
                    via[it->first] = node;
                    cost[it->first] = newCost;
                    pq.push(PQEntry(it->first, it->second));
                }
            }
        }
    }

    // Getting next hop
    for (int j = 0; j < size; j++) {
        if (inc[j]) {
            if (via[j] == number)
                next[j] = j;
            else {
                int k = via[j];
                set<int>::iterator it = neighbors.find(k);
                //cerr << "j = " << j << ",via = " << via[j] << ", inc = " << inc[j] << ", cost = " << cost[j] << endl;
                for (; it == neighbors.end(); k = via[k], it = neighbors.find(k));
                next[j] = k;

            }
        }
    }

    routing_table.setTable(number, inc, next, cost, size);
}

/*void LinkState::SendMessage(int originalSender, Link& l) {
    RoutingMessage* m = new RoutingMessage(this->number, l);

    deque<Node*>* neighbors = GetNeighbors();
    deque<Node*>::iterator it = neighbors->begin();
    for (; it != neighbors->end(); it++)
        if (originalSender != (*it)->GetNumber()) {
            SendToNeighbor(*it, m);
        }
}*/
