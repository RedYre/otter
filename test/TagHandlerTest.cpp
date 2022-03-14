#include "gtest/gtest.h"
#include "TagHandler.h"
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>

TEST(TagHandlerTest, testoutput) {

  osmium::io::File input_file{"/container/test/test.osm"};
  osmium::io::Reader reader{input_file};
  char* arguments[] = {
    (char*)("./otter"),
    (char*)("-osmpath"),
    (char*)("/container/test/test.osm"),
    (char*)("-ttlpath"),
    (char*)("/container/bin/test.gz")
  };
  Otter::Configurator cfg(5, arguments);
  Otter::TagHandler handler(cfg);
  osmium::apply(reader, handler);
  handler.close();
  reader.close();
  // decompress and read the output file
  std::ifstream file("/container/bin/test.gz", std::ios_base::in | std::ios_base::binary);
  boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
  inbuf.push(boost::iostreams::bzip2_decompressor());
  inbuf.push(file);
  //Convert streambuf to istream
  std::istream instream(&inbuf);
  std::string line;
  std::vector<std::string> lines;
  while(std::getline(instream, line)) {
    lines.push_back(line);
  }
  EXPECT_TRUE(lines.size() > 2);
  std::cout << lines[0] << "\n";
  std::cout << lines[lines.size() / 2] << "\n";
  std::cout << lines[lines.size() - 1] << "\n";
  EXPECT_TRUE(lines[0] == 
    "@prefix osmnode: <https://www.openstreetmap.org/node/> .");
  EXPECT_TRUE(lines[lines.size() / 2] == 
    "osmnode:3 osmt:name \"Bremen-Neustadt\" .");
  EXPECT_TRUE(lines[lines.size() - 1] == 
    "osmrel:1000 osmt:type \"restriction\" .");
  file.close();
}





