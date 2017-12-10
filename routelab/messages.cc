#include "messages.h"

RoutingMessage::RoutingMessage()
{}

RoutingMessage::RoutingMessage(const RoutingMessage &rhs) {
    *this = rhs;
}

RoutingMessage & RoutingMessage::operator=(const RoutingMessage & rhs) {
    /* For now.  Change if you add data members to the struct */
    return *this;
}

#if defined(GENERIC)
ostream &RoutingMessage::Print(ostream &os) const
{
    os << "Generic RoutingMessage()";
    return os;
}
#endif

#if defined(LINKSTATE)

RoutingMessage::RoutingMessage(int s, Link& l) : sender(s), link(l) {}

ostream &RoutingMessage::Print(ostream &os) const
{
    os << "LinkState RoutingMessage(" << sender << "," << link.GetSrc() << "," << link.GetDest() << "," << link.GetLatency() << ")";
    return os;
}
#endif

#if defined(DISTANCEVECTOR)

RoutingMessage::RoutingMessage(int sender, map<int, TopoLink> dv) {
    sendingNode = sender;
    distanceVector = dv;
}
ostream &RoutingMessage::Print(ostream &os) const
{
    os << "DistanceVector RoutingMessage()";
    return os;
}
#endif
