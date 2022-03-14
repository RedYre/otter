#ifndef STATHANDLER_H_
#define STATHANDLER_H_

#include "Configurator.h"
#include "Util.h"
#include "Structs_STXXL.h"

#include <cstdint>
#include <fstream>
#include <iostream>

#include <osmium/handler.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/way.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

/*
 * sweep through the dataset and compute statistics
 * stats are used for reserving and namecluster IDs
 */
namespace Otter
{
  class StatHandler : public osmium::handler::Handler
  {
  public:
    StatHandler();

    void node(const osmium::Node & /*node*/);
    void way(const osmium::Way & /*way*/);
    void relation(const osmium::Relation & /*relation*/);

    // interface override functions to handle events
    long long int nodes = 0;
    long long int ways = 0;
    long long int maxway = 0;

    // members to count statistics(main purpose of this handler)
    long long int relations = 0;
    long long int countRelRels = 0;
    long long int countRelsNoRels = 0;
    long long int counttypemultipolygon = 0;
    long long int counttypemultipolygonandrelrel = 0;
    long long int counttypeboundary = 0;
    // more strict conditions
    long long int countdoublcond = 0;
    long long int counttriplecond = 0;
    long long int countquadracond = 0;
    long long int counttypemultipolygonandtypeboundary = 0;
  };
}
#endif // STATHANDLER_H_
