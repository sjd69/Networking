#include <map>
#include <utility>

#include "linkstate.h"

LinkState::LinkState(unsigned n, SimulationContext* c, double b, double l) :
    Node(n, c, b, l)
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

    if (UpdateGraph(number, dest, latency)) {
        Dijkstra();
        SendToNeighbors(new RoutingMessage(l));
    }
}

void LinkState::ProcessIncomingRoutingMessage(RoutingMessage *m) {
    cerr << *this << " got a routing message: " << *m << " (ignored)" << endl;

    Link* l = m->link;
    double latency = (*l).GetLatency();
    int src = (*l).GetSrc();
    int dest = (*l).GetDest();

    if (UpdateGraph(src, dest, latency)) {
        Dijkstra();
        SendToNeighbors(new RoutingMessage(*m));
    }
}

void LinkState::TimeOut() {
    cerr << *this << " got a timeout: (ignored)" << endl;
}

Node* LinkState::GetNextHop(Node *destination) {
    return NULL;
}

Table* LinkState::GetRoutingTable() {
    return NULL;
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
            return old != latency;
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
    // TODO
}
