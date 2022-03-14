#include "TagHandler.h"

void Otter::TagHandler::ttlprefixes()
{
  // write the prefixes for the ttl file
  std::string delim = conf.delim;
  outstream[0] << "@prefix" << delim << conf.subjPrefixNode << delim << "<https://www.openstreetmap.org/node/>" << delim << ".\n";
  outstream[0] << "@prefix" << delim << conf.subjPrefixWay << delim << "<https://www.openstreetmap.org/way/>" << delim << ".\n";
  outstream[0] << "@prefix" << delim << conf.subjPrefixRelation << delim << "<https://www.openstreetmap.org/relation/>" << delim << ".\n";
  outstream[0] << "@prefix" << delim << conf.predPrefixDefault << delim << "<https://www.wiki.openstreetmap.org/wiki/key:>" << delim << ".\n";
  outstream[0] << "@prefix" << delim << conf.osmwiki << delim << "<https://wiki.openstreetmap.org/wiki/>" << delim << ".\n";
  outstream[0] << "@prefix" << delim << conf.geof << delim << "<http://www.opengis.net/def/function/geosparql/>" << delim << ".\n";
  outstream[0] << "@prefix" << delim << conf.geo << delim << "<http://www.opengis.net/ont/geosparql#>" << delim << ".\n";
  outstream[0] << "@prefix" << delim << conf.objPrefixWikipedia << delim << "<https://de.wikipedia.org/wiki/>" << delim << ".\n";
  outstream[0] << "@prefix" << delim << conf.objPrefixWikidata << delim << "<http://www.wikidata.org/entity/>" << delim << ".\n";
}

Otter::TagHandler::TagHandler(Otter::Configurator cfg)
{
  ofstr = new std::ofstream[1];
  ofstr[0].open(cfg.dataoutpathV, std::ofstream::binary | std::ofstream::trunc);
  outstream = new boost::iostreams::filtering_ostream[1];
  outstream[0].push(boost::iostreams::bzip2_compressor());
  outstream[0].push(ofstr[0]);

  conf = cfg;
  TagHandler::ttlprefixes();
}

void Otter::TagHandler::close()
{
  outstream[0].pop();
  ofstr[0].flush();
  if (ofstr[0].is_open())
  {
    ofstr[0].close();
  }
}

// This callback is called by osmium::apply for each node in the data.
void Otter::TagHandler::node(const osmium::Node &node)
{

  tagsToTriplesTTL(node.tags(), conf.subjPrefixNode +
                                    std::to_string(node.id()));

  std::string coordstring;

  coordstring.reserve(88);
  coordstring += conf.subjPrefixNode;
  coordstring += std::to_string(node.id());
  coordstring += conf.wktnodecenter;
  coordstring += std::to_string(node.location().lon());
  coordstring += " ";
  coordstring += std::to_string(node.location().lat());
  coordstring += conf.wktnodesuffix;

  outstream[0] << coordstring;
}

// This callback is called by osmium::apply for each way in the data.
void Otter::TagHandler::way(const osmium::Way &way)
{
  tagsToTriplesTTL(way.tags(), conf.subjPrefixWay + std::to_string(way.id()));
}

// This callback is called by osmium::apply for each relation in the data.
void Otter::TagHandler::relation(const osmium::Relation &relation)
{
  tagsToTriplesTTL(relation.tags(), conf.subjPrefixRelation + std::to_string(relation.id()));
}

// rules for ttlconversion:
// key contains "wikidata" && key does not contain "note" -> split by ';' and print the triples wrt the split with "wd:" pred prefix
// key contains "wikipedia" && key does not contain "note" -> use "wpd:" as predicate prefix
// key equals "is_in" && value contain "," -> split and remove leading space
// value starts with "http" or "www" -> use <$(value)>
// default: use "$(value)"
inline void Otter::TagHandler::tagsToTriplesTTL(const osmium::TagList &tags, std::string subject)
{

  // use subject as common prefix of the tags
  subject += conf.delim;
  subject += conf.predPrefixDefault;

  for (const osmium::Tag &tag : tags)
  {
    std::string key(tag.key());
    std::string value(tag.value());
    key = url_encode(key);

    if (key.find("wiki") != std::string::npos)
    {
      if (key.find("wikidata") != std::string::npos && value.at(0) == 'Q' && key.find("note") == std::string::npos)
      {
        // split by ";" and use each object
        std::string trimmed = replaceAll(value, " ", "");
        for (const std::string &each : cppsplit(trimmed, ";"))
        {
          std::string tostream;
          tostream.reserve(subject.length() + key.length() +
                           conf.delim.length() + conf.objPrefixWikidata.length() +
                           each.length() + conf.delim.length() +
                           conf.triplend.length());
          tostream += subject;

          tostream += key;
          tostream += conf.delim;
          tostream += conf.objPrefixWikidata;
          tostream += each;
          tostream += conf.triplend;

          outstream[0] << tostream;
        }
      }
      else if (key.find("wikipedia") != std::string::npos && key.find("note") == std::string::npos)
      {
        if (value.size() > 4)
        {
          if (value.substr(0, 4) == "http")
          {
            // use < >

            std::string tostream;
            tostream.reserve(subject.length() + key.length() +
                             2 * conf.delim.length() + 2 + value.length() +
                             conf.triplend.length());
            tostream += subject;
            tostream += key;
            tostream += conf.delim;
            tostream += "<";
            tostream += replaceAll(value, " ", "_");
            tostream += ">";
            tostream += conf.triplend;
            outstream[0] << tostream;
          }
          else
          {

            std::string tostream;
            tostream.reserve(subject.length() + key.length() +
                             2 * conf.delim.length() + 12 + value.length() +
                             conf.triplend.length());
            tostream += subject;
            tostream += key;
            tostream += conf.delim;
            tostream += conf.objPrefixWikipedia;
            tostream += url_encode(replaceAll(value, " ", "_"));
            tostream += conf.triplend;
            outstream[0] << tostream;
          }
        }
        else
        {

          std::string tostream;
          tostream.reserve(subject.length() + key.length() +
                           2 * conf.delim.length() + 12 + value.length() +
                           conf.triplend.length());
          tostream += subject;
          tostream += key;
          tostream += conf.delim;
          tostream += conf.objPrefixWikipedia;
          tostream += url_encode(replaceAll(value, " ", "_"));
          tostream += conf.triplend;
          outstream[0] << tostream;
        }
      }
    }
    else if (key == "is_in")
    {
      // trim spaces and split by "," since is_in is often multi-valued
      std::string trimmed = replaceAll(value, " ", "");

      for (const std::string &each : cppsplit(trimmed, ";"))
      {
        std::string tostream;
        tostream.reserve(subject.length() + key.length() +
                         2 * conf.delim.length() + 2 + value.length() +
                         conf.triplend.length());
        tostream += subject;
        tostream += key;
        tostream += conf.delim;
        tostream += "\"";
        tostream += each;
        tostream += "\"";
        tostream += conf.triplend;
        outstream[0] << tostream;
      }
    }
    else if (value.size() > 3 &&
             (value.substr(0, 4) == "http" ||
              value.substr(0, 4) == "www.") &&
             key.find("note") == std::string::npos)
    {
      // try to intepret as link

      std::string tostream;
      tostream.reserve(subject.length() + key.length() +
                       2 * conf.delim.length() + 10 + value.length() +
                       conf.triplend.length());
      tostream += subject;
      tostream += key;
      tostream += conf.delim;
      tostream += "<";
      tostream += url_encode(replaceAll(value, " ", "_"));
      tostream += ">";
      tostream += conf.triplend;
      outstream[0] << tostream;
    }
    else
    {
      encodeLiteral(value);
      std::string tostream;
      tostream.reserve(subject.length() + key.length() +
                       2 * conf.delim.length() + 2 + value.length() +
                       conf.triplend.length());
      tostream += subject;
      tostream += key;
      tostream += conf.delim;
      tostream += "\"";
      tostream += value;
      tostream += "\"";
      tostream += conf.triplend;
      outstream[0] << tostream;
    }
  }
}
