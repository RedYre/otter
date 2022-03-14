#ifndef TAGHANDLER_H_
#define TAGHANDLER_H_

#include "Configurator.h"
#include "Util.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string_view>

#include <osmium/handler.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/way.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include "gtest/gtest.h"

/*
 * this class writes all tags as well as the coords of nodes to the output file
 */
namespace Otter
{
  class TagHandler : public osmium::handler::Handler
  {
  public:
    // the constructor prepares the outputstream
    TagHandler(Otter::Configurator cfg);

    // node , way and relation are override functions used by the implemented
    // osmium::handler
    void node(const osmium::Node & /*node*/);
    void way(const osmium::Way & /*way*/);
    void relation(const osmium::Relation & /*relation*/);

    // closes the output
    void close();

    // constructs the string containing the tag triples
    inline void tagsToTriplesTTL(const osmium::TagList &tags, std::string subject);

  private:
    FRIEND_TEST(TagHandlerTest, testoutput);

    // used to write output
    std::ofstream *ofstr;
    boost::iostreams::filtering_ostream *outstream;

    // the passed config
    Otter::Configurator conf;

    // writes tll prefixes to output, used in the constructor
    void ttlprefixes();
  };
}
#endif // TAGHANDLER_H_
