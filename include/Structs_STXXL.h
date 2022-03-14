#ifndef STRUCTS_STXXL_H_
#define STRUCTS_STXXL_H_

/*
 * these structs are used to hold data in the on-disk vectors(STXXL)
 */

struct NodeCoord
{
  NodeCoord(long long int id, double nlon, double nlat) : ID(id), lon(nlon), lat(nlat) {}
  NodeCoord(long long int id) : ID(id) {}
  NodeCoord() {}

  long long int ID;
  double lon;
  double lat;
};

struct WayRefs
{
  WayRefs(long long int ID, int order, long long int refID, int32_t nameID, bool tag) : ID(ID), order(order), refID(refID), nameID(nameID), hastag(tag) {}
  WayRefs(long long int id) : ID(id) {}
  WayRefs() {}
  long long int ID;
  int order;
  long long int refID;
  int32_t nameID;
  bool isPolygon;
  bool hastag;
};

struct WayCoord
{
  WayCoord(long long int id, int ord, double nlon, double nlat, int32_t nameid, bool tag) : ID(id), order(ord), lon(nlon), lat(nlat), nameID(nameid), hastag(tag) {}
  WayCoord(long long int id) : ID(id) {}
  WayCoord() {}
  long long int ID;
  int order;
  double lat;
  double lon;
  int32_t nameID;
  bool isPolygon;
  bool hastag;
};

struct RelRefs
{
  RelRefs(long long int id, int ord, char t, long long int rid,
          int32_t nameid, char r, bool tag) : ID(id), order(ord), type(t), refID(rid), nameID(nameid),
                                              role(r), hastag(tag) {}
  RelRefs(long long int rid) : refID(rid) {}
  RelRefs() {}
  long long int ID;
  int order;
  char type; // n, w, r
  long long int refID;
  int32_t nameID;
  char role; // 0 is inner, 1 is subarea, 2 is outer, 3 is empty, 4 is anything else
  bool hastag;
};

struct RelCoord
{
  RelCoord(long long int id, int ord, int subord, char t,
           double lo, double la, int32_t nid, char rol, bool tag) : ID(id), order(ord), suborder(subord), type(t),
                                                                    lon(lo), lat(la), nameID(nid), role(rol), hastag(tag) {}
  RelCoord() {}
  RelCoord(long long int id, int ord, int subord) : ID(id), order(ord), suborder(subord) {}
  long long int ID;
  int order;
  int suborder;
  char type; // n, w, r
  double lon;
  double lat;
  int32_t nameID;
  char role; // 0 is inner, 1 is subarea, 2 is outer, 3 is empty, 4 is anything else
  bool hastag;
};

struct Coordless
{
  long long int ID;
  // char type; // n, w, r
  long long int mappedGrid;
};

struct NameCluster
{
  NameCluster(long long int i, long long int ri) : ID(i), refID(ri) {}
  NameCluster() {}
  long long int ID;
  long long int refID;
};

struct Tagpair
{
  Tagpair(std::string a, std::string b) : k(a), v(b) {}
  Tagpair() {}
  std::string k;
  std::string v;
};

// achieve an ascending order of sorting
struct nodeCmptr
{
  bool operator()(const NodeCoord &a, const NodeCoord &b) const
  {
    return a.ID < b.ID;
  }
  NodeCoord min_value() const
  {
    return NodeCoord(std::numeric_limits<long long int>::min());
  }
  NodeCoord max_value() const
  {
    return NodeCoord(std::numeric_limits<long long int>::max());
  }
};

// achieve an ascending order of sorting
struct coordlessByMappedGrid
{
  bool operator()(const Coordless &a, const Coordless &b) const
  {
    return a.mappedGrid < b.mappedGrid;
  }
  Coordless min_value() const
  {
    Coordless nc;
    nc.mappedGrid = std::numeric_limits<long long int>::min();
    return nc;
  }
  Coordless max_value() const
  {
    Coordless nc;
    nc.mappedGrid = std::numeric_limits<long long int>::max();
    return nc;
  }
};

// achieve an ascending order of sorting
struct coordlessByMappedID
{
  bool operator()(const Coordless &a, const Coordless &b) const
  {
    return a.ID < b.ID;
  }
  Coordless min_value() const
  {
    Coordless nc;
    nc.ID = std::numeric_limits<long long int>::min();
    return nc;
  }
  Coordless max_value() const
  {
    Coordless nc;
    nc.ID = std::numeric_limits<long long int>::max();
    return nc;
  }
};

// achieve an ascending order of sorting
struct wayCmptr
{
  bool operator()(const WayRefs &a, const WayRefs &b) const
  {
    return a.refID < b.refID;
  }
  WayRefs min_value() const
  {
    return WayRefs(std::numeric_limits<long long int>::min());
  }
  WayRefs max_value() const
  {
    return WayRefs(std::numeric_limits<long long int>::max());
  }
};

// primarily sort by nameID, then refID
struct wayrefbyNameRefid
{
  bool operator()(const WayRefs &a, const WayRefs &b) const
  {
    if (a.nameID != b.nameID)
    {
      return a.nameID < b.nameID;
    }
    return a.refID < b.refID;
  }
  WayRefs min_value() const
  {
    return WayRefs(0, 0, std::numeric_limits<long long int>::min(),
                   std::numeric_limits<int32_t>::min(), false);
  }
  WayRefs max_value() const
  {
    return WayRefs(0, 0, std::numeric_limits<long long int>::max(),
                   std::numeric_limits<int32_t>::max(), false);
  }
};

// achieve an ascending order of sorting
struct wayRCmptr
{
  bool operator()(const WayCoord &a, const WayCoord &b) const
  {
    if (a.ID != b.ID)
    {
      return a.ID < b.ID;
    }
    return a.order < b.order;
  }
  WayCoord min_value() const
  {
    return WayCoord(std::numeric_limits<long long int>::min());
  }
  WayCoord max_value() const
  {
    return WayCoord(std::numeric_limits<long long int>::max());
  }
};

// sort by ID and order
struct wayCoordIDorder
{
  bool operator()(const WayCoord &a, const WayCoord &b) const
  {
    if (a.ID != b.ID)
    {
      return a.ID < b.ID;
    }
    return a.order < b.order;
  }
  WayCoord min_value() const
  {
    WayCoord nc;
    nc.ID = std::numeric_limits<long long int>::min();
    nc.order = std::numeric_limits<int>::min();
    return nc;
  }
  WayCoord max_value() const
  {
    WayCoord nc;
    nc.ID = std::numeric_limits<long long int>::max();
    nc.order = std::numeric_limits<int>::max();
    return nc;
  }
};

// achieve an ascending order of sorting
struct relsByID
{
  bool operator()(const RelCoord &a, const RelCoord &b) const
  {
    return a.ID < b.ID;
  }
  RelCoord min_value() const
  {
    RelCoord nc;
    nc.ID = std::numeric_limits<long long int>::min();
    return nc;
  }
  RelCoord max_value() const
  {
    RelCoord nc;
    nc.ID = std::numeric_limits<long long int>::max();
    return nc;
  }
};

// achieve an ascending order of sorting
struct wayRCmptrOrder
{
  bool operator()(const WayCoord &a, const WayCoord &b) const
  {
    return a.order < b.order;
  }
  WayCoord min_value() const
  {
    WayCoord nc;
    nc.order = std::numeric_limits<int>::min();
    return nc;
  }
  WayCoord max_value() const
  {
    WayCoord nc;
    nc.order = std::numeric_limits<int>::max();
    return nc;
  }
};

// achieve an ascending order of sorting
struct relByOrder
{
  bool operator()(const RelCoord &a, const RelCoord &b) const
  {
    if (a.order != b.order)
    {
      return a.order < b.order;
    }
    return a.suborder < b.suborder;
  }
  RelCoord min_value() const
  {
    RelCoord nc;
    nc.order = std::numeric_limits<int>::min();
    nc.suborder = std::numeric_limits<int>::min();
    return nc;
  }
  RelCoord max_value() const
  {
    RelCoord nc;
    nc.order = std::numeric_limits<int>::max();
    nc.suborder = std::numeric_limits<int>::max();
    return nc;
  }
};

// by id, order and suborder
struct relcoordbyidordersuborder
{
  bool operator()(const RelCoord &a, const RelCoord &b) const
  {
    if (a.ID != b.ID)
    {
      return a.ID < b.ID;
    }
    if (a.order != b.order)
    {
      return a.order < b.order;
    }
    return a.suborder < b.suborder;
  }
  RelCoord min_value() const
  {
    return RelCoord(std::numeric_limits<long long int>::min(),
                    std::numeric_limits<int>::min(),
                    std::numeric_limits<int>::min());
  }
  RelCoord max_value() const
  {
    return RelCoord(std::numeric_limits<long long int>::max(),
                    std::numeric_limits<int>::max(),
                    std::numeric_limits<int>::max());
  }
};

// achieve an ascending order of sorting
struct wayRCmptrLat
{
  bool operator()(const WayCoord &a, const WayCoord &b) const
  {
    return a.lat < b.lat;
  }
  WayCoord min_value() const
  {
    WayCoord nc;
    nc.lat = std::numeric_limits<double>::min();
    return nc;
  }
  WayCoord max_value() const
  {
    WayCoord nc;
    nc.lat = std::numeric_limits<double>::max();
    return nc;
  }
};

// achieve an ascending order of sorting
struct wayRCmptrLon
{
  bool operator()(const WayCoord &a, const WayCoord &b) const
  {
    return a.lon < b.lon;
  }
  WayCoord min_value() const
  {
    WayCoord nc;
    nc.lon = std::numeric_limits<double>::min();
    return nc;
  }
  WayCoord max_value() const
  {
    WayCoord nc;
    nc.lon = std::numeric_limits<double>::max();
    return nc;
  }
};

// achieve an ascending order of sorting
struct wayRCmptrName
{
  bool operator()(const WayCoord &a, const WayCoord &b) const
  {
    return a.nameID < b.nameID;
  }
  WayCoord min_value() const
  {
    WayCoord nc;
    nc.nameID = std::numeric_limits<size_t>::min();
    return nc;
  }
  WayCoord max_value() const
  {
    WayCoord nc;
    nc.nameID = std::numeric_limits<int32_t>::max();
    return nc;
  }
};

struct relrefbyrefid
{
  bool operator()(const RelRefs &a, const RelRefs &b) const
  {
    return a.refID < b.refID;
  }
  RelRefs min_value() const
  {
    return RelRefs(std::numeric_limits<long long int>::min());
  }
  RelRefs max_value() const
  {
    return RelRefs(std::numeric_limits<long long int>::max());
  }
};

//
struct relsByIDByType
{
  bool operator()(const RelRefs &a, const RelRefs &b) const
  {
    if (a.type != b.type)
    {
      if (a.type == 'r')
      {
        return true;
      }
      if (b.type == 'r')
      {
        return false;
      }
    }
    return a.ID < b.ID;
  }
  RelRefs min_value() const
  {
    RelRefs nc;
    nc.ID = std::numeric_limits<size_t>::min();
    nc.type = 'r';
    return nc;
  }
  RelRefs max_value() const
  {
    RelRefs nc;
    nc.ID = std::numeric_limits<size_t>::max();
    nc.type = 'n';
    return nc;
  }
};

struct orderWCVec
{
  inline bool operator()(const WayCoord &struct1, const WayCoord &struct2)
  {
    return (struct1.order < struct2.order);
  }
};

struct orderCoordless
{
  inline bool operator()(const Coordless &struct1, const Coordless &struct2)
  {
    return (struct1.mappedGrid < struct2.mappedGrid);
  }
};

#endif // STRUCTS_STXXL_H_
