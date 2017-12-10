#ifndef _table
#define _table

#include <iostream>
#include <map>

using namespace std;

struct TopoLink {
    TopoLink(): cost(-1), age(0) {}

    TopoLink(const TopoLink & rhs) {
        *this = rhs;
    }

    TopoLink & operator=(const TopoLink & rhs) {
        this->cost = rhs.cost;
        this->age = rhs.age;

        return *this;
    }

    int cost;
    int age;
};

// Students should write this class
class Table {
    private:

    public:
        Table();
        Table(const Table &);
        Table & operator=(const Table &);

        ostream & Print(ostream &os) const;

        // Anything else you need
        map < int, map < int, TopoLink > > topo;

        #if defined(LINKSTATE)
            map <int, int> topoMap;

            bool topoMapChanged;
        #endif

        #if defined(DISTANCEVECTOR)
            map <int, TopoLink> distanceVector;
            map <int, TopoLink> neighborLinks;
            map <int, int> topoMap;
        #endif
};

inline ostream & operator<<(ostream &os, const Table & t) { return t.Print(os);}


#endif
