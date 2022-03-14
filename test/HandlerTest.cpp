#include "gtest/gtest.h"
#include "BoundaryHandler.h"
#include "StatHandler.h"
#include "LocHandler.h"
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>

TEST(HandlerTest, generateRTREE)
{
  // first use stat handler to get stats
  // then starting boundary handler in order
  // to compute boundaries(osm rels with boundary.. tags)
  // then use the location handler and check if elements are
  // within the boundary of the testset

  // this testfile contains a boundary in the shape of a rectangle
  osmium::io::File input_file{"/container/test/test2.osm"};

  // stathandler code
  osmium::io::Reader read{input_file};
  Otter::StatHandler sh;
  osmium::apply(read, sh);
  read.close();

  // boundary handler code
  osmium::io::Reader reader{input_file};
  char *arguments[] = {
      (char *)("./otter"),
      (char *)("-osmpath"),
      (char *)("/container/test/test2.osm"),
      (char *)("-ttlpath"),
      (char *)("/container/bin/test2.gz")};
  Otter::Configurator cfg(5, arguments);
  Otter::BoundaryHandler bh;
  // read rels
  osmium::io::Reader relationreader{input_file, osmium::osm_entity_bits::relation};
  osmium::apply(relationreader, bh);
  relationreader.close();

  // read ways
  std::sort(bh.usedways.begin(), bh.usedways.end());
  osmium::io::Reader wayreader{input_file, osmium::osm_entity_bits::way};
  osmium::apply(wayreader, bh);
  wayreader.close();

  // read nodes
  std::sort(bh.usednodes.begin(), bh.usednodes.end());
  osmium::io::Reader nodereader{input_file, osmium::osm_entity_bits::node};
  osmium::apply(nodereader, bh);
  nodereader.close();
  bh.usednodes.clear();

  // prepare for nodes and ways for resolving with zipping algorithm
  std::sort(bh.nodecoords.begin(), bh.nodecoords.end(), nodeCmptr());
  std::sort(bh.wayrefs.begin(), bh.wayrefs.end(), wayCmptr());
  bh.computewaycoords();

  // clear and sort stuff
  bh.nodecoords.clear();
  bh.wayrefs.clear();
  std::sort(bh.boundaries.begin(), bh.boundaries.end(), relrefbyrefid());
  std::sort(bh.waycoords.begin(), bh.waycoords.end(), wayCoordIDorder());
  bh.resolveboundaries();
  bh.boundaries.clear();
  bh.waycoords.clear();

  // actual assembly
  std::sort(bh.boundariesresolved.begin(), bh.boundariesresolved.end(), relcoordbyidordersuborder());
  bh.assemblegeometry();

  // gererate directed acyclic graph out of RTREE
  bh.genrtree();
  bh.sortpackingmembers();
  bh.genDAG();
  reader.close();

  std::cout << "DAGSIZE:" << bh.DAG.size() << "\n";
  std::cout << "rtree:" << bh.rtree.size() << "\n";

  // introduce LocHandler
  osmium::io::Reader r{input_file};
  // store nodes, wayrefs and relrefs
  Otter::LocHandler lh(cfg, sh);
  lh.DAG.swap(bh.DAG);
  lh.rtree.swap(bh.rtree);
  lh.open();
  osmium::apply(r, lh);
  lh.close();
  {
    // process ways
    lh.sortNodes();
    lh.sortWays();
    lh.mergeNandW();
    {
      // compute waycluster
      lh.sortWayrefsByRefID();
      lh.findconnectedcomponents();
      lh.enablenameclusters();
      lh.armnamedclusters();
      osmium::io::Reader r{input_file};
      lh.open();
      osmium::apply(r, lh);
      lh.close();
      r.close();
      lh.sortWaynodes();
      lh.writenameclustergeometry();
      // check for namecluster
    }
    lh.sortWaynodes();
    lh.writeways();
  }
  {
    // process relation geometry
    lh.sortmultiypolygonrefs();
    lh.resolvenodesinmultipolygons();
    {
      lh.nonsortmultiypolygonrefs();
      lh.resolvenodesinnonmultipolygons();
    }
    lh.resolvewaysinmultipolygons();
    {
      lh.resolvewaysinnonmultipolygons();
    }
    lh.sortmultipolygon();
    lh.writemultipolygonrels();
    {
      lh.sortnonmultipolygon();
      lh.writenonmultipolygonrels();
    }
  }
  lh.close();

  // decompress the output and read it in
  std::ifstream file("/container/bin/test2.gz", std::ios_base::in | std::ios_base::binary);
  boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
  inbuf.push(boost::iostreams::bzip2_decompressor());
  inbuf.push(file);
  // Convert streambuf to istream
  std::istream instream(&inbuf);
  std::string line;
  std::vector<std::string> lines;
  while (std::getline(instream, line))
  {
    lines.push_back(line);
  }
  file.close();

  // compare outputlines with
  std::ifstream file2("/container/test/test2.ttl");
  std::string str;
  std::vector<std::string> linesttl;
  while (std::getline(file2, str))
  {
    linesttl.push_back(str);
  }

  bool sameoutput = true;
  if (linesttl.size() != lines.size())
  {
    sameoutput = false;
  }
  for (unsigned int i = 0; i < lines.size(); i += 1)
  {
    if (lines[i].compare(linesttl[i].substr(0, linesttl[i].size() - 1)) != 0)
    {
      sameoutput = false;
    }
  }
  // view test2.ttl to check the output
  // this tests nameclusters, the construction of all geometries, and their spatial relations
  // also tags were written, so the whole execution pipeline was used
  EXPECT_TRUE(sameoutput);
  EXPECT_TRUE(lines.size() == 64);
}
