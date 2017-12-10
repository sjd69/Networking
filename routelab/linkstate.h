#ifndef _linkstate
#define _linkstate

#include "node.h"

class LinkState: public Node {
    private:
        // Anything you need in addition to Node members
        map<int, map<int, double> > graph;
        map<pair<int,int>, int> seen;
        map<pair<int, int>, int> current;

    public:
        LinkState(unsigned, SimulationContext* , double, double);
        LinkState(const LinkState &);
        LinkState & operator=(const LinkState &);
        ~LinkState();

        // Inherited from Node
        void LinkHasBeenUpdated(Link *l);
        void ProcessIncomingRoutingMessage(RoutingMessage *m);
        void TimeOut();
        Node* GetNextHop(Node* destination);
        Table* GetRoutingTable();
        ostream & Print(ostream & os) const;

        // Anything else
        void Dijkstra();
        bool UpdateGraph(int src, int dest, double latency);
        void SendMessage(int sender, Link& l);
};

inline ostream & operator<<(ostream & os, const LinkState & n) {
    return n.Print(os);
}

#endif
