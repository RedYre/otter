#ifndef Configurator_H_
#define Configurator_H_

#include "ArgumentOption.h"
#include "ArgumentParser.h"

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <sys/stat.h>

#include <unordered_map>
#include "gtest/gtest.h"

namespace Otter
{
    class Configurator
    {
    public:
        // the path of the Configurator file and the program arguments are passed
        Configurator(int argc, char *argv[]);
        Configurator();
        // Configurator(const Configurator& copyme);
        Configurator &operator=(Configurator other);

        // delimiter for the output file
        std::string delim = " ";
        std::string triplend = delim + ".\n";

        // dont output data, only count stats
        const bool onlystats = false;

        //  buffer for sorting stxxl vector
        const long long int sortbuffer = 512LL * 1024 * 1024 * 4 * 2LL;

        // listing conceptual terms for the Configurator variables to
        // these can be seen as keys for the values of the matched vars
        std::string datainpathK = "";
        std::string dataoutpathK = "";
        std::string helpArg = "";

        // the actual paths are found here
        std::string datainpathV = "";
        std::string dataoutpathV = "";

        // var names of rdf terms
        std::string predPrefixDefault = "osmt:";
        std::string objPrefixWikidata = "wd:";
        std::string objPrefixWikipedia = "wpd:";
        std::string subjPrefixNode = "osmnode:";
        std::string subjPrefixWay = "osmway:";
        std::string subjPrefixRelation = "osmrel:";
        std::string osmwiki = "osmwiki:";
        std::string geof = "geof:";
        std::string geo = "geo:";
        std::string hasGeometry = "hasGeometry";
        std::string geometrycollection = "\"GEOMETRYCOLLECTION(";
        std::string point = "POINT(";
        std::string linestring = "LINESTRING(";
        std::string polygon = "POLYGON((";
        std::string contains = "sfContains";     // contains
        std::string within = "sfWithin";         // within
        std::string intersects = "sfIntersects"; // physical connection/intersection
        std::string wktSuffix =
            ")\"^^geo:wktLiteral";

        // compoundstring
        // concatenated middle part of geometry triples
        std::string wktnodecenter =
            delim + geo + hasGeometry + delim + "\"" + point;
        std::string wktrelcenter =
            delim + geo + hasGeometry + delim + geometrycollection;
        // concatenation of last strings of geometries
        std::string wktgeometrys =
            ")" + wktSuffix + delim + ".\n";
        // concatenation of last strings of points
        std::string wktnodesuffix =
            wktSuffix + delim + ".\n";
        std::string wktsuffixgc =
            wktSuffix + delim + ".\n";
        // constant middle part of "is part of" relation
        std::string ispartofmiddlepart = delim + geof + within + delim;
        std::string ispartofmiddlepartreverse = delim + geof + contains + delim;

        std::string intersectsmiddlepart = delim + geof + intersects + delim;

        FRIEND_TEST(Configurator, fileExists);
        FRIEND_TEST(Configurator, containsArg);

        // check if file exists
        bool fileExists(std::string pfile);
        // check if the option is there and matched
        bool containsArg(std::string name);

    private:
        // List of Configurator args
        std::vector<Otter::ArgumentOption> argOptions = {
            Otter::ArgumentOption("osmpath", "The path to the input file.", 1, Otter::ArgumentOption::Type::STRING),
            Otter::ArgumentOption("ttlpath", "The path to the output file.", 1, Otter::ArgumentOption::Type::STRING),
            Otter::ArgumentOption("help", "Print the program description.")};



        // check if the option is there and matched
        Otter::ArgumentOption getArg(std::string name);
    };
}
#endif // Configurator_H_
