#ifndef READER_H_
#define READER_H_

#include "TagHandler.h"
#include "LocHandler.h"
#include "Configurator.h"
#include "StatHandler.h"
#include "BoundaryHandler.h"
#include "Timer.h"

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>

#include <stxxl/vector>
// #include <stxxl/sorter>

/*
 * Read in via libosmium
 * The main processing is done here
 */
namespace Otter
{
  class Reader
  {
  public:
    Reader(Configurator cfg);

  private:
    // Configurator conf;
  };
}
#endif // READER_H_
