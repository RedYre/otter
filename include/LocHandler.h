#ifndef LOCHANDLER_H_
#define LOCHANDLER_H_

#include "Configurator.h"
#include "Util.h"
#include "Structs_STXXL.h"
#include "StatHandler.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <unistd.h>

#include <osmium/handler.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/way.hpp>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <stxxl/vector>
#include <stxxl/sort>
#include "gtest/gtest.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree>> Point;
typedef bg::model::box<Point> Box;
typedef bg::model::polygon<Point, false, true> Polygon; // ccw, open polygon
typedef bg::model::multi_polygon<Polygon> Multipolygon;
typedef bg::model::segment<Point> Segment;
typedef bg::model::linestring<Point> Linestring;

typedef std::tuple<Box, Multipolygon, Multipolygon, Multipolygon, long long int, float> Boundpoly;
typedef bg::model::ring<Point> Ring;

/*
 * storing nodes, way-references and relation-references here and resolve the refs
 */
namespace Otter
{
  class LocHandler : public osmium::handler::Handler
  {
  public:
    LocHandler(Otter::Configurator cfg, StatHandler s);
    LocHandler(){};
    /*
     * overrides of the handler, they define how to handle when an OSM node/way/rel is found
     */
    void node(const osmium::Node & /*node*/);
    void way(const osmium::Way & /*way*/);
    void relation(const osmium::Relation & /*relation*/);

    // resolve wayrefs to waycoords by mergin with nodecoords
    void mergeNandW();

    // sorting methods
    void sortNodes();
    void sortWays();
    void sortWaynodes();
    void sortWayrefsByRefID();

    // assemble & process waycoords and write to output
    void writeways();

    // open and close file for writing output
    void close();
    void open();

    // compute connected component for nameclusters
    void findconnectedcomponents();

    // signalize to only listen to way events when reading input
    // and process ways differently
    void enablenameclusters();

    // prepare maps for computing nameclusters
    void armnamedclusters();

    // writing passed tags wrt passe subject string to output
    void tagsToTriplesTTL(const std::vector<Tagpair> &tags, std::string subject);

    // clear maps used for nameclusters
    void clearncmaps();

    // write geometry of nameclusters to output
    void writenameclustergeometry();

    // functions for computing and writing osm relations
    void resolvenodesinmultipolygons();
    void resolvenodesinnonmultipolygons();
    void resolvewaysinmultipolygons();
    void resolvewaysinnonmultipolygons();
    void sortmultiypolygonrefs();
    void nonsortmultiypolygonrefs();
    void sortmultipolygon();
    void sortnonmultipolygon();
    void writemultipolygonrels();
    void writenonmultipolygonrels();

    // write geometry of a relation by passing nodes, outer and inner ring and id
    void writesinglerel(std::vector<RelCoord> &relnode,
                        std::vector<std::vector<RelCoord>> &outer,
                        std::vector<std::vector<RelCoord>> &inner,
                        long long int relid);

    // store nodes, they are the basis of the OSM geometries
    stxxl::VECTOR_GENERATOR<NodeCoord>::result nodes;

    // ways do point to multiple nodes, here we store the entries of wayID and nodeID
    stxxl::VECTOR_GENERATOR<WayRefs>::result wayrefs;
    stxxl::VECTOR_GENERATOR<WayCoord>::result waynodes;

    /*
     * process first rels without subrels
     * for those, compute first multipolygons, then
     * non multipolygons
     * we dont look at rels that contain other rels
     * because very few rels contain other rels
     */
    // multipolygon relations: ~50% of rels
    stxxl::VECTOR_GENERATOR<RelRefs>::result multipolygonrefs;
    stxxl::VECTOR_GENERATOR<RelCoord>::result multipolygoncoords;

    // non-multipolygon relations: remaining ~50% of rels
    stxxl::VECTOR_GENERATOR<RelRefs>::result nonmultipolygonrefs;
    stxxl::VECTOR_GENERATOR<RelCoord>::result nonmultipolygoncoords;

    // map/index of wayref.nameID
    std::unordered_map<std::string, size_t> nametoid;
    std::unordered_map<size_t, std::string> idtoname;

    // counting vars
    long long int countwrittenmultipolygons = 0;
    long long int countwrittennonmultipolygons = 0;

    // reparation stats
    long long int countearlyaccept = 0;
    long long int countearlyreject = 0;
    long long int countregular = 0;

    // error tolerance for polygon
    long long int countpolytolerancesuccessful = 0;
    long long int countpolytolerancefailed = 0;

    // number of nameclusters
    long long int countNameCluster = 0;

    // swap with the prior computed DAG
    std::unordered_map<long long int, std::vector<long long int>> DAG;

    // the RTREE from boundaryhandler is swapped to lochandler
    boost::geometry::index::rtree<Boundpoly,
                                  boost::geometry::index::quadratic<16>>
        rtree;

  private:
    FRIEND_TEST(LocHandler, testoutput);

    // used when computing connected component
    void procwayfragmentchunk(std::vector<WayRefs> &chunk);

    // used to count how many entries of waycoords were processed
    long long int synccountway = 0;

    // copy count from stat handler and use it to reserve for a data structure
    std::uint64_t waycount = 0;

    // used for setting IDs of nameclusters, since they are new data points
    std::uint64_t maxcount = 0;

    // offset on top of maxwaycount
    long long int nameclusteroffset = 0;

    // store newly generated way ID and refID(other ways)
    std::vector<NameCluster> nameclusters;
    // used to enable nameclustering instead of usual reader
    bool computenameclusters = false;
    std::unordered_map<long long int, std::vector<Tagpair>> nctags;
    std::unordered_map<long long int, std::vector<long long int>> ncindex;
    std::unordered_map<long long int, std::vector<long long int>> ncindexreverse;
    // use NameCluster just as a pair of numbers, ID is the actual value
    // and refID is the target value(terminating condition)
    std::unordered_map<long long int, NameCluster> ncfinished;
    std::unordered_map<long long int, std::vector<std::vector<double>>> refIDtolons;
    std::unordered_map<long long int, std::vector<std::vector<double>>> refIDtolats;

    // true if second passed poly is inside the first passed poly
    bool polycontainspoly(const std::vector<RelCoord> &outer, const std::vector<RelCoord> &inner);

    // used to write to output
    std::ofstream *ofstr;
    boost::iostreams::filtering_ostream *outstream;

    // store passed config args
    Otter::Configurator conf;

    // used for DAG processing
    std::vector<long long int> findsucc(long long int source);
    void findsuccrec(long long int source, std::vector<long long int> &sucs);

    // used to compute the containment relation(spatial relation)
    void linestringwithin(Linestring lines, std::string IDstring);
    void polywithin(Polygon poly, std::string IDstring);
    void geocollectionwithin(std::vector<Point> gpoints,
                             std::vector<Linestring> glines,
                             std::vector<Polygon> gpolys, std::string ID);
  };
}
#endif // LOCHANDLER_H_
