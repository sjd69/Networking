#include "table.h"

Table::Table(const Table & rhs) {
    *this = rhs;
}

Table & Table::operator=(const Table & rhs) {
    /* For now,  Change if you add more data members to the class */
    topo = rhs.topo;

    return *this;
}

#if defined(GENERIC)
ostream & Table::Print(ostream &os) const
{
  os << "Generic Table()";
  return os;
}

Table::Table() {
    topo.clear();
}
#endif

#if defined(LINKSTATE)
ostream & Table::Print(ostream &os) const
{
  os << "LinkState Table()";
  return os;
}

Table::Table() {
    // TODO
}
#endif

#if defined(DISTANCEVECTOR)
ostream & Table::Print(ostream &os) const
{
  os << "DistanceVector Table()";
  return os;
}

Table::Table() {
    topo.clear();
    distanceVector.clear();
    neighborLinks.clear();
    topoMap.clear();
}
#endif
