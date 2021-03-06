#ifndef _messages
#define _messages

#include <iostream>
#include "node.h"
#include "link.h"

struct RoutingMessage {
    RoutingMessage();
    RoutingMessage(const RoutingMessage &rhs);
    RoutingMessage &operator=(const RoutingMessage &rhs);

    ostream & Print(ostream &os) const;

    // Anything else you need

    #if defined(LINKSTATE)
        RoutingMessage(pair<int, int>, int, Link& l);
        pair<int, int> originator;
        int sequence;
        Link link;
    #endif
    #if defined(DISTANCEVECTOR)
        RoutingMessage(int sender, map <int, TopoLink> dv);
        int sendingNode;
        map<int, TopoLink> distanceVector;
    #endif
};

inline ostream & operator<<(ostream &os, const RoutingMessage & m) { return m.Print(os);}

#endif
