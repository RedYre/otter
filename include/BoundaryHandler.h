#ifndef BOUNDARYHANDLER_H_
#define BOUNDARYHANDLER_H_

#include "Configurator.h"
#include "Util.h"
#include "Structs_STXXL.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <cmath>

#include <osmium/handler.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/way.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point.hpp>

#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/index/rtree.hpp>
#include "gtest/gtest.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree>> Point;
typedef bg::model::box<Point> Box;
typedef bg::model::polygon<Point, false, true> Polygon; // ccw, open polygon
typedef bg::model::multi_polygon<Polygon> Multipolygon;
typedef bg::model::segment<Point> Segment;
typedef std::tuple<Box, Multipolygon, Multipolygon, Multipolygon, long long int, float> Boundpoly;
typedef bg::model::ring<Point> Ring;

namespace Otter
{
  class BoundaryHandler : public osmium::handler::Handler
  {
  public:
    BoundaryHandler();

    // node way and relation overrides are used for the osmium::handler interface
    // one defines what happens if an OSM element is found
    void node(const osmium::Node & /*node*/);
    void way(const osmium::Way & /*way*/);
    void relation(const osmium::Relation & /*relation*/);

    // compute waycoords from sorted wayrefs and nodecoords
    void computewaycoords();

    // compute boundaries(osm elements with type=boundary...), resolve their references
    void resolveboundaries();

    // after boundaries are resolved, the entries are assembled to the whole geometry and then processed
    void assemblegeometry();

    // the generation of the RTREE data structure
    void genrtree();

    // generate directed acyclic graph based on RTREE
    void genDAG();

    // sort packingmembers(those are the geometries of boundaries, which are fed into the RTREE)
    void sortpackingmembers();

    // convert omodes to boost::polygon
    Polygon relcoordstopolygon(std::vector<RelCoord> omodes);

    // attempt on resolving self-intersection of a polygon, used after point-reduction
    void resolveintersection(Polygon &in);

    // compute underestimated polygon with reduced number of points
    void reducepointsinner(Polygon in, Multipolygon &out, float areainit);

    // compute overestimated polygon with reduced number of nodes
    void reducepointsouter(Polygon input, Multipolygon &output);

    // due to the application of the douglas peucker algorithm, the last element needs to be reappended
    void reappendlastpoint(Multipolygon &poly);
    void reappendlastpoint(Polygon &poly);

    // compute score for ranking vs other polygons
    float scoreofpoly(size_t maxpoint, float areainit,
                      size_t currentpoint, float currentarea);

    // assemble and process polygon by providing outer and inner rings
    void genpoly(/*std::vector<RelCoord>& relnode,*/
                 std::vector<std::vector<RelCoord>> &outer,
                 std::vector<std::vector<RelCoord>> &inner,
                 long long int relid);

    // true if second passed poly is inside the first passed poly
    bool polycontainspoly(const std::vector<RelCoord> &outer, const std::vector<RelCoord> &inner);

    // douglas peucker algorithm to calculate inner & outer simplified polys
    void simplifiedpolygon(std::vector<RelCoord> inputPoints,
                           Multipolygon &outputPoints, size_t l, size_t r,
                           double eps, bool inside);
    void simplifiedpolygon(Polygon inputPoints,
                           Multipolygon &outputPoints, size_t l, size_t r,
                           double eps, bool inside);

    // used in douglas peucker in order to calc perpendicular distance
    double perpendiculardistance(RelCoord A, RelCoord B, RelCoord C);
    // used in douglas peucker in order to calc perpendicular distance
    double perpendiculardistance(Point A, Point B, Point C);

    //  ID, wayrefID, role,order,...
    std::vector<RelRefs> boundaries;
    // same as boundaries entries, but lon and lat instead of wayRefID
    std::vector<RelCoord> boundariesresolved;

    // each entry contains ID and one referred node of a way
    std::vector<WayRefs> wayrefs;

    // same as wayrefs, but resolved references with coords
    std::vector<WayCoord> waycoords;

    // each entry contains the node and coords of it
    // its the basis of the gemotric data
    std::vector<NodeCoord> nodecoords;

    // IDs which are remembered for computing boundaries
    std::vector<long long int> usedways;
    // IDs which are remembered for computing boundaries
    std::vector<long long int> usednodes;

    // geo index structure
    boost::geometry::index::rtree<Boundpoly,
                                  boost::geometry::index::quadratic<16>>
        rtree;

    // "type=boundary"-polygons which are fed into the RTREE for init
    std::vector<Boundpoly> packingmembers;

    // number of polygons added and not added
    size_t countAddPoly = 0;
    size_t countNotAddPoly = 0;

    // number of Boundaries
    size_t countBoundary = 0;

    // count number of reparations of overestimation
    size_t countRepairOverestimateSuccess = 0;
    size_t countRepairOverestimateFail = 0;
    size_t countOverestimate = 0;
    float sumrelativepointreductionOver = 0;
    size_t numberrelativepointreductionOver = 0;

    // count number of reparations of underestimation
    size_t countRepairUnderestimateSuccess = 0;
    size_t countRepairUnderestimateFail = 0;
    size_t countUnderestimate = 0;
    float sumrelativepointreductionUnder = 0;
    size_t numberrelativepointreductionUnder = 0;

    // counting how often a threshold is exceeded for the number points of a polygon
    size_t nBelowThresh = 0;
    size_t nAboveThresh = 0;

    // used to compute mean
    long long int totalPointsBoundary = 0;
    size_t numberPointsBoundary = 0;

    // DAG... map ID to ref IDs
    std::unordered_map<long long int, std::vector<long long int>> DAG;

    // used for DAG
    std::vector<long long int> findsucc(long long int source);
    void findsuccrec(long long int source, std::vector<long long int> &sucs);
  };

#endif // BOUNDARYHANDLER_H_
}