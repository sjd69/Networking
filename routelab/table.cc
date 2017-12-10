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
  os << "LinkState Table(";
  map<int, int>::const_iterator it = via.begin();
  for (; it != via.end(); it++)
    os << it->first << ":" << it->second << ",";
  os << ")" << endl;
  return os;
}

Table::Table() {
    topo.clear();
}

void Table::setTable(int origin, bool* inc, int* viaA, double* costA, int size) {
    int i;

    via.clear();
    cost.clear();

    for (i = 0; i < size; i++) {
        if (inc[i]) {
            pair<int, int> viaPair (i,
                viaA[i] == origin ? i : viaA[i]);
            pair<int, double> costPair (i, costA[i]);
            via.insert(viaPair);
            cost.insert(costPair);
        }
    }
}

int Table::getNextHop(int dest) {
    return via[dest];
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
