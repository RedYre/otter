#include "LocHandler.h"

Otter::LocHandler::LocHandler(Otter::Configurator cfg, StatHandler s)
{

  waycount = s.ways;
  maxcount = s.maxway;
  conf = cfg;

  // heuristic for reserve.. approx 17M of name values
  size_t waycountmax = 17000000;
  nametoid.reserve((waycount * 100) / waycountmax);
  idtoname.reserve((waycount * 100) / waycountmax);
  // init mapping of wayref.nameID
  nametoid[""] = 0;
  idtoname[0] = "";
}

void Otter::LocHandler::close()
{
  if (!conf.onlystats)
  {
    outstream[0].pop();
    ofstr[0].flush();
    if (ofstr[0].is_open())
    {
      ofstr[0].close();
    }
  }
}

void Otter::LocHandler::open()
{
  if (!conf.onlystats)
  {
    ofstr = new std::ofstream[1];
    ofstr[0].open(conf.dataoutpathV, std::ofstream::binary | std::ofstream::app);
    outstream = new boost::iostreams::filtering_ostream[1];
    outstream[0].push(boost::iostreams::bzip2_compressor());
    outstream[0].push(ofstr[0]);
  }
}

void Otter::LocHandler::clearncmaps()
{
  nctags.clear();
  ncindex.clear();
  ncfinished.clear();
  refIDtolons.clear();
  refIDtolats.clear();
}

void Otter::LocHandler::writenameclustergeometry()
{
  // loop over nameclusters and init
  clearncmaps();
  ncindex.reserve(nameclusteroffset);
  ncindexreverse.reserve(nameclusteroffset);
  refIDtolons.reserve(nameclusteroffset);
  refIDtolats.reserve(nameclusteroffset);
  // collect the wayrefs, then output
  for (unsigned int i = 0; i < nameclusters.size(); i += 1)
  {
    ncindex[nameclusters[i].refID].push_back(nameclusters[i].ID);
    ncindexreverse[nameclusters[i].ID].push_back(nameclusters[i].refID);
  }

  nameclusters.clear();
  // read waynodes and accumulate coords per ID
  stxxl::VECTOR_GENERATOR<WayCoord>::result::bufreader_type reader(waynodes);
  std::vector<double> lons;
  std::vector<double> lats;
  long long int lastID = -1;
  synccountway = 0;

  open();

  // write which ways the new way consists of
  for (auto const &[key, val] : ncindexreverse)
  {
    // contains
    std::string writeme = conf.subjPrefixWay;
    writeme.reserve(55);
    writeme += std::to_string(key);
    writeme += conf.delim;
    writeme += conf.geof;
    writeme += conf.contains;
    writeme += conf.delim;
    writeme += conf.subjPrefixWay;
    for (long long int oldway : val)
    {
      if (!conf.onlystats)
      {
        outstream[0] << (writeme + std::to_string(oldway) + conf.triplend);
      }
    }
  }

  bool isIDrelevant = false;
  // write geometry of namecluster-ways
  while (!reader.empty())
  {

    while (!reader.empty())
    {
      // check if I even want to store lon lat
      if (ncindex.contains((*reader).ID))
      {
        isIDrelevant = true;
      }
      if (isIDrelevant && lastID != (*reader).ID)
      {
        // need to process lons lats before proceeding
        break;
      }

      if (isIDrelevant)
      {
        lons.push_back((*reader).lon);
        lats.push_back((*reader).lat);
      }
      ++reader;
    }

    if (!isIDrelevant)
    {
      continue;
    }
    else
    {
      // at start, do update lastID
      if (lons.size() == 0)
      {
        lastID = (*reader).ID;
      }
      if (isIDrelevant && lons.size() > 0)
      {
        //  process lons lats and update the maps

        if (lons.size() > 0)
        {
          // loop over map with used node ID as key
          std::vector<long long int> temp;
          for (long long int ncrev : ncindex[lastID])
          {
            // associate new way ID(from namecluster) with the used coords
            refIDtolons[ncrev].push_back(lons);
            refIDtolats[ncrev].push_back(lats);

            if (ncindexreverse[ncrev].size() ==
                    refIDtolons[ncrev].size() &&
                ncindexreverse[ncrev].size() ==
                    refIDtolats[ncrev].size())
            {
              // write geometry
              std::string writeme = conf.subjPrefixWay;
              writeme.reserve(80 + 12 + (200 * refIDtolons[ncrev].size()));
              writeme += std::to_string(ncrev);
              writeme += conf.delim;
              writeme += conf.geo;
              writeme += conf.hasGeometry;
              writeme += conf.delim;
              writeme += conf.geometrycollection;

              for (unsigned int i = 0; i < refIDtolons[ncrev].size(); i += 1)
              {
                bool ispolygon = false;
                if (refIDtolons[ncrev][i].size() > 3)
                {

                  if (refIDtolons[ncrev][i][0] ==
                          refIDtolons[ncrev][i][refIDtolons[ncrev][i].size() - 1] &&
                      refIDtolats[ncrev][i][0] ==
                          refIDtolats[ncrev][i][refIDtolats[ncrev][i].size() - 1])
                  {
                    writeme += conf.polygon;

                    ispolygon = true;
                  }
                  else
                  {
                    writeme += conf.linestring;
                  }
                }
                else if (refIDtolons[ncrev].size() > 1)
                {
                  writeme += conf.linestring;
                }

                // print coords
                for (unsigned int j = 0; j < refIDtolons[ncrev][i].size(); j += 1)
                {
                  writeme += std::to_string(refIDtolons[ncrev][i][j]);
                  writeme += conf.delim;
                  writeme += std::to_string(refIDtolats[ncrev][i][j]);
                  if (j < refIDtolons[ncrev][i].size() - 1)
                  {
                    writeme += ",";
                  }
                }
                if (ispolygon)
                {
                  writeme += ")";
                }
                writeme += ")";
                if (i < refIDtolons[ncrev].size() - 1)
                {
                  writeme += ",";
                }
              }
              writeme += conf.wktSuffix;
              writeme += conf.delim;
              writeme += ".\n";

              if (!conf.onlystats)
              {
                outstream[0] << writeme;
              }
              ncindexreverse.erase(ncrev);
              refIDtolons.erase(ncrev);
              refIDtolats.erase(ncrev);
            }
            else
            {
              temp.push_back(ncrev);
            }
          }

          // erase ncrev(if statement)from ncindex
          if (temp.size() == 0)
          {
            ncindex.erase(lastID);
          }
          else
          {
            ncindex[lastID] = temp;
          }
        }

        lats.clear();
        lons.clear();
      }
      isIDrelevant = false;
      if (!reader.empty())
      {
        lastID = (*reader).ID;
      }
    }
  }
  clearncmaps();
  synccountway = 0;
  close();
}

// This callback is called by osmium::apply for each node in the data.
void Otter::LocHandler::node(const osmium::Node &node)
{
  if (computenameclusters)
  {
    return;
  }
  nodes.push_back(NodeCoord(node.id(), node.location().lon(), node.location().lat()));

  // compute is-within relation
  // check if node has name tag

  if (false && node.tags().size() > 0)
  {
    Point pnode = Point(node.location().lon(), node.location().lat());

    // rtree query
    std::vector<Boundpoly> result;
    rtree.query(bg::index::covers(pnode), std::back_inserter(result));

    // sort packingmembers
    std::sort(result.begin(), result.end(),
              [this](Boundpoly &a, Boundpoly &b)
              {
                return std::get<5>(a) < std::get<5>(b);
              });
    std::vector<long long int> skipset;

    const long long int &nodeID = node.id();

    for (const Boundpoly &res : result)
    {

      const long long int &resID = std::get<4>(res);

      if (std::find(skipset.begin(), skipset.end(), resID) != skipset.end())
      {
        continue;
      }

      // check if in underestimation
      if (bg::covered_by(pnode, std::get<3>(res)))
      {
        // check successor set via DAG
        std::vector<long long int> succs = findsucc(resID);
        succs.push_back(resID);
        countearlyaccept++;

        std::string stringID = std::to_string(nodeID);
        std::vector<std::string> printme;
        printme.reserve((succs.size() / 2) * 10);
        for (const long long int &succ : succs)
        {
          if (std::find(skipset.begin(), skipset.end(), succ) == skipset.end())
          {
            printme.push_back(conf.subjPrefixNode);
            printme.push_back(stringID);
            printme.push_back(conf.ispartofmiddlepart);
            printme.push_back(conf.subjPrefixRelation);
            printme.push_back(std::to_string(succ));
            printme.push_back(conf.triplend);

            printme.push_back(conf.subjPrefixRelation);
            printme.push_back(std::to_string(succ));
            printme.push_back(conf.ispartofmiddlepartreverse);
            printme.push_back(conf.subjPrefixNode);
            printme.push_back(stringID);
            printme.push_back(conf.triplend);

            printme.push_back(conf.subjPrefixNode);
            printme.push_back(stringID);
            printme.push_back(conf.intersectsmiddlepart);
            printme.push_back(conf.subjPrefixRelation);
            printme.push_back(std::to_string(succ));
            printme.push_back(conf.triplend);

            printme.push_back(conf.subjPrefixRelation);
            printme.push_back(std::to_string(succ));
            printme.push_back(conf.intersectsmiddlepart);
            printme.push_back(conf.subjPrefixNode);
            printme.push_back(stringID);
            printme.push_back(conf.triplend);
          }
        }

        if (!conf.onlystats && printme.size() > 0)
        {
          outstream[0] << std::accumulate(printme.begin(), printme.end(), std::string{});
        }

        skipset.insert(skipset.end(), succs.begin(), succs.end());
      }
      else
      {
        // check if in overestimation

        if (bg::covered_by(pnode, std::get<2>(res)))
        {
          // check if in actual
          countregular++;
          if (bg::covered_by(pnode, std::get<1>(res)))
          {

            // check successor set via DAG
            std::vector<long long int> succs = findsucc(resID);
            succs.push_back(resID);

            std::string stringID = std::to_string(nodeID);
            std::vector<std::string> printme;
            printme.reserve((succs.size() / 2) * 10);

            for (const long long int &succ : succs)
            {

              if (std::find(skipset.begin(), skipset.end(), succ) == skipset.end())
              {

                printme.push_back(conf.subjPrefixNode);
                printme.push_back(stringID);
                printme.push_back(conf.ispartofmiddlepart);
                printme.push_back(conf.subjPrefixRelation);
                printme.push_back(std::to_string(succ));
                printme.push_back(conf.triplend);

                printme.push_back(conf.subjPrefixRelation);
                printme.push_back(std::to_string(succ));
                printme.push_back(conf.ispartofmiddlepartreverse);
                printme.push_back(conf.subjPrefixNode);
                printme.push_back(stringID);
                printme.push_back(conf.triplend);

                printme.push_back(conf.subjPrefixNode);
                printme.push_back(stringID);
                printme.push_back(conf.intersectsmiddlepart);
                printme.push_back(conf.subjPrefixRelation);
                printme.push_back(std::to_string(succ));
                printme.push_back(conf.triplend);

                printme.push_back(conf.subjPrefixRelation);
                printme.push_back(std::to_string(succ));
                printme.push_back(conf.intersectsmiddlepart);
                printme.push_back(conf.subjPrefixNode);
                printme.push_back(stringID);
                printme.push_back(conf.triplend);
              }
            }
            skipset.insert(skipset.end(), succs.begin(), succs.end());

            if (!conf.onlystats && printme.size() > 0)
            {
              outstream[0] << std::accumulate(printme.begin(), printme.end(), std::string{});
            }
          }
        }
        else
        {
          countearlyreject++;
        }
      }
    }
  }
}

void Otter::LocHandler::armnamedclusters()
{

  // count generated waysclusters
  long long int countnc = 0;
  long long int lastid = 0;
  if (nameclusters.size() > 0)
  {
    countnc++;
    lastid = nameclusters[0].ID;
  }
  for (long long int i = 0; i < nameclusters.size(); i += 1)
  {
    if (lastid != nameclusters[i].ID)
    {
      countnc++;
      lastid = nameclusters[i].ID;
    }
  }

  nctags.reserve(countnc);
  ncindex.reserve(countnc);
  ncfinished.reserve(countnc);
  // init ncindex
  for (long long int i = 0; i < nameclusters.size(); i += 1)
  {
    if (ncindex.contains(nameclusters[i].refID))
    {
      if (std::find(ncindex[nameclusters[i].refID].begin(),
                    ncindex[nameclusters[i].refID].end(),
                    nameclusters[i].ID) == ncindex[nameclusters[i].refID].end())
      {
        ncindex[nameclusters[i].refID].push_back(nameclusters[i].ID);
      }
    }
    else
    {
      ncindex[nameclusters[i].refID] = std::vector<long long int>{nameclusters[i].ID};
    }
  }
  // init ncfinished
  countnc = 0;
  lastid = 0;
  if (nameclusters.size() > 0)
  {
    lastid = nameclusters[0].ID;
  }
  for (long long int i = 0; i < nameclusters.size(); i += 1)
  {
    if (lastid != nameclusters[i].ID)
    {
      ncfinished[lastid] = NameCluster(0, countnc);
      countnc = 0;
    }
    lastid = nameclusters[i].ID;
    countnc++;
    if (i == nameclusters.size() - 1)
    {
      ncfinished[lastid] = NameCluster(0, countnc);
    }
  }
}

// This callback is called by osmium::apply for each way in the data.
void Otter::LocHandler::way(const osmium::Way &way)
{
  if (computenameclusters)
  {
    if (conf.onlystats)
    {
      return;
    }
    if (ncindex.contains(way.id()))
    {
      for (long long int index : ncindex[way.id()])
      {
        // // was init before, now filter
        if (nctags.contains(index))
        {
          // // conjunction of tags, no disjunctions
          std::vector<Tagpair> sharedtags;
          for (Tagpair tp : nctags[index])
          {
            bool shared = false;
            for (const auto &tag : way.tags())
            {
              if (tp.k == tag.key() && tp.v == tag.value())
              {
                shared = true;
              }
            }
            if (shared)
            {
              sharedtags.push_back(tp);
            }
          }
          nctags[index] = sharedtags;
          if (ncfinished.contains(index))
          {
            ncfinished[index].ID++;
          }
          else
          {
            std::cout << "1:processed element, which should not be processed.\n";
            std::cout << index << "\n";
          }
        }
        else
        {
          // first init of nctags[index]
          for (const auto &tag : way.tags())
          {
            nctags[index].push_back(Tagpair(tag.key(), tag.value()));
          }
          if (ncfinished.contains(index))
          {
            ncfinished[index].ID++;
          }
          else
          {
            std::cout << "2:processed element, which should not be processed.\n";
            std::cout << index << "\n";
          }
        }
        // termination condition? then print
        if (ncfinished.contains(index))
        {
          if (ncfinished[index].ID == ncfinished[index].refID)
          {
            tagsToTriplesTTL(nctags[index],
                             conf.subjPrefixWay +
                                 std::to_string(index));
            countNameCluster++;
          }
        }
      }
    }
    return;
  }

  int order = 0;
  size_t ni = 0;
  if (way.tags().has_key("name"))
  {

    if (nametoid.contains(way.tags()["name"]))
    {
      ni = nametoid[way.tags()["name"]];
    }
    else
    {
      size_t nameidsize = nametoid.size();
      idtoname[nameidsize] = way.tags()["name"];
      nametoid[way.tags()["name"]] = nameidsize;
      ni = nameidsize;
    }
  }
  bool hastags = way.tags().size() > 0;
  for (const osmium::NodeRef &nr : way.nodes())
  {
    wayrefs.push_back(WayRefs(way.id(), order, nr.ref(), ni, hastags));
    order++;
  }
}

void Otter::LocHandler::resolvewaysinmultipolygons()
{
  // resolve multipolygonrefs wrt ways
  stxxl::VECTOR_GENERATOR<WayCoord>::result::bufreader_type readerW(waynodes);
  stxxl::VECTOR_GENERATOR<RelRefs>::result::bufreader_type readerR(multipolygonrefs);
  multipolygonrefs.reserve(multipolygonrefs.size());
  stxxl::VECTOR_GENERATOR<RelCoord>::result::bufwriter_type writer(multipolygoncoords.end());

  std::vector<WayCoord> waymembers;
  while (!readerW.empty() && !readerR.empty())
  {
    if ((*readerW).ID == (*readerR).refID)
    {
      if ((*readerR).type == 'w')
      {
        long long int actid = (*readerW).ID;
        while (!readerW.empty())
        {
          if ((*readerW).ID != actid)
          {
            break;
          }
          waymembers.push_back((*readerW));
          ++readerW;
        }
        while (!readerR.empty())
        {
          if ((*readerR).refID != actid)
          {
            break;
          }
          for (WayCoord wc : waymembers)
          {
            writer << RelCoord((*readerR).ID, (*readerR).order, wc.order, 'w',
                               wc.lon, wc.lat, (*readerR).nameID, (*readerR).role, (*readerR).hastag);
          }
          ++readerR;
        }
        waymembers.clear();
        continue;
      }
      ++readerR;
    }
    else
    {
      if ((*readerR).refID < (*readerW).ID)
      {
        ++readerR;
      }
      else
      {
        ++readerW;
      }
    }
  }
  writer.finish();
}

void Otter::LocHandler::resolvewaysinnonmultipolygons()
{

  // resolve multipolygonrefs wrt ways
  stxxl::VECTOR_GENERATOR<WayCoord>::result::bufreader_type readerW(waynodes);
  stxxl::VECTOR_GENERATOR<RelRefs>::result::bufreader_type readerR(nonmultipolygonrefs);
  nonmultipolygoncoords.reserve(nonmultipolygonrefs.size());
  stxxl::VECTOR_GENERATOR<RelCoord>::result::bufwriter_type writer(nonmultipolygoncoords.end());

  std::vector<WayCoord> waymembers;
  while (!readerW.empty() && !readerR.empty())
  {
    if ((*readerW).ID == (*readerR).refID)
    {
      if ((*readerR).type == 'w')
      {
        long long int actid = (*readerW).ID;
        while (!readerW.empty())
        {
          if ((*readerW).ID != actid)
          {
            break;
          }
          waymembers.push_back((*readerW));
          ++readerW;
        }
        while (!readerR.empty())
        {
          if ((*readerR).refID != actid)
          {
            break;
          }
          // process waymembers with (*readerR)
          for (WayCoord wc : waymembers)
          {
            writer << RelCoord((*readerR).ID, (*readerR).order, wc.order, 'w',
                               wc.lon, wc.lat, (*readerR).nameID, (*readerR).role, (*readerR).hastag);
          }
          ++readerR;
        }
        waymembers.clear();
        continue;
      }
      ++readerR;
    }
    else
    {
      if ((*readerR).refID < (*readerW).ID)
      {
        ++readerR;
      }
      else
      {
        ++readerW;
      }
    }
  }
  writer.finish();
}

void Otter::LocHandler::resolvenodesinnonmultipolygons()
{

  // resolve multipolygonrefs wrt nodes
  stxxl::VECTOR_GENERATOR<NodeCoord>::result::bufreader_type readerN(nodes);
  stxxl::VECTOR_GENERATOR<RelRefs>::result::bufreader_type readerR(nonmultipolygonrefs);
  nonmultipolygoncoords.reserve(nonmultipolygonrefs.size());
  stxxl::VECTOR_GENERATOR<RelCoord>::result::bufwriter_type writer(nonmultipolygoncoords);

  while (!readerN.empty() && !readerR.empty())
  {
    if ((*readerN).ID == (*readerR).refID)
    {
      if ((*readerR).type == 'n')
      {
        writer << RelCoord((*readerR).ID, (*readerR).order, 0, 'n',
                           (*readerN).lon, (*readerN).lat, (*readerR).nameID, (*readerR).role, (*readerR).hastag);
      }
      ++readerR;
    }
    else
    {
      if ((*readerR).refID < (*readerN).ID)
      {
        ++readerR;
      }
      else
      {
        ++readerN;
      }
    }
  }
  writer.finish();
}

void Otter::LocHandler::resolvenodesinmultipolygons()
{

  // resolve multipolygonrefs wrt nodes
  stxxl::VECTOR_GENERATOR<NodeCoord>::result::bufreader_type readerN(nodes);
  stxxl::VECTOR_GENERATOR<RelRefs>::result::bufreader_type readerR(multipolygonrefs);
  multipolygoncoords.reserve(multipolygonrefs.size());
  stxxl::VECTOR_GENERATOR<RelCoord>::result::bufwriter_type writer(multipolygoncoords);
  while (!readerN.empty() && !readerR.empty())
  {
    if ((*readerN).ID == (*readerR).refID)
    {
      if ((*readerR).type == 'n')
      {
        writer << RelCoord((*readerR).ID, (*readerR).order, 0, 'n',
                           (*readerN).lon, (*readerN).lat, (*readerR).nameID, (*readerR).role, (*readerR).hastag);
      }
      ++readerR;
    }
    else
    {
      if ((*readerR).refID < (*readerN).ID)
      {
        ++readerR;
      }
      else
      {
        ++readerN;
      }
    }
  }
  writer.finish();
}

// This callback is called by osmium::apply for each relation in the data.
void Otter::LocHandler::relation(const osmium::Relation &relation)
{
  if (computenameclusters)
  {
    return;
  }

  int order = 0;
  size_t ni = 0;
  bool hastag = relation.tags().size() > 0;
  if (relation.tags().has_key("name"))
  {

    if (nametoid.contains(relation.tags()["name"]))
    {
      ni = nametoid[relation.tags()["name"]];
    }
    else
    {
      size_t nameidsize = nametoid.size();
      idtoname[nameidsize] = relation.tags()["name"];
      nametoid[relation.tags()["name"]] = nameidsize;
      ni = nameidsize;
    }
  }

  // check if relation has subrelation
  bool isrelrel = false;
  for (const osmium::RelationMember &rm : relation.members())
  {
    if (rm.type() == osmium::item_type::relation)
    {
      isrelrel = true;
    }
  }

  for (const osmium::RelationMember &rm : relation.members())
  {
    char t = rm.type() == osmium::item_type::relation ? 'r' : (rm.type() == osmium::item_type::node ? 'n' : (rm.type() == osmium::item_type::way ? 'w' : 'x'));
    if (t == 'x')
    {
      std::cout << "relation member type not node/way/relation(skip element)\n";
      continue;
    }

    // encode role in the following:
    // 0 is inner, 1 is subarea, 2 is outer, 3 is empty, 4 is anything else
    char role = '4';
    if (strcmp(rm.role(), "inner") == 0)
    {
      role = '0';
    }
    else if (strcmp(rm.role(), "subarea") == 0)
    {
      role = '1';
    }
    else if (strcmp(rm.role(), "outer") == 0)
    {
      role = '2';
    }
    else if (strcmp(rm.role(), "") == 0)
    {
      role = '3';
    }

    if (relation.tags().has_tag("type", "multipolygon"))
    {
      multipolygonrefs.push_back(RelRefs(relation.id(), order, t, rm.ref(),
                                         ni, role, hastag));
    }
    else
    {
      if (!isrelrel)
      {
        nonmultipolygonrefs.push_back(RelRefs(relation.id(), order, t, rm.ref(),
                                              ni, role, hastag));
      }
    }
    order++;
  }
}

void Otter::LocHandler::sortmultipolygon()
{
  stxxl::sort(multipolygoncoords.begin(), multipolygoncoords.end(),
              relcoordbyidordersuborder(), conf.sortbuffer);
}

void Otter::LocHandler::sortnonmultipolygon()
{
  stxxl::sort(nonmultipolygoncoords.begin(), nonmultipolygoncoords.end(),
              relcoordbyidordersuborder(), conf.sortbuffer);
}

void Otter::LocHandler::sortmultiypolygonrefs()
{
  stxxl::sort(multipolygonrefs.begin(), multipolygonrefs.end(),
              relrefbyrefid(), conf.sortbuffer);
}

void Otter::LocHandler::nonsortmultiypolygonrefs()
{
  stxxl::sort(nonmultipolygonrefs.begin(), nonmultipolygonrefs.end(),
              relrefbyrefid(), conf.sortbuffer);
}

void Otter::LocHandler::sortNodes()
{
  stxxl::sort(nodes.begin(), nodes.end(), nodeCmptr(), conf.sortbuffer);
}

void Otter::LocHandler::sortWays()
{
  stxxl::sort(wayrefs.begin(), wayrefs.end(), wayCmptr(), conf.sortbuffer);
}

void Otter::LocHandler::sortWaynodes()
{
  stxxl::sort(waynodes.begin(), waynodes.end(), wayRCmptr(), conf.sortbuffer);
}

void Otter::LocHandler::sortWayrefsByRefID()
{
  stxxl::sort(wayrefs.begin(), wayrefs.end(), wayrefbyNameRefid(), conf.sortbuffer);
}

void Otter::LocHandler::enablenameclusters()
{
  computenameclusters = true;
}

void Otter::LocHandler::findconnectedcomponents()
{
  // wayrefs are sorted by nameID and refID
  // a) get nameID chunk and compute its connected components
  stxxl::VECTOR_GENERATOR<WayRefs>::result::bufreader_type reader(wayrefs);
  std::vector<WayRefs> chunk;
  size_t ni = 0;
  if (!reader.empty())
  {
    ni = (*reader).nameID;
  }
  while (!reader.empty())
  {
    if ((*reader).nameID == 0)
    {
      ++reader;
      continue;
    }

    if ((*reader).nameID != ni)
    {
      // process chunk
      if (chunk.size() > 0)
      {
        procwayfragmentchunk(chunk);
      }
      chunk.clear();
    }
    chunk.push_back(*reader);
    ni = (*reader).nameID;
    ++reader;
  }
  if (chunk.size() > 0)
  {
    // process chunk
    procwayfragmentchunk(chunk);
  }
  std::cout << "nameclusters.size(): " << nameclusters.size() << "\n";
}

void Otter::LocHandler::procwayfragmentchunk(std::vector<WayRefs> &chunk)
{
  // track IDs that are connected to another ID
  // use tracked IDs as filter, work only with those

  std::vector<std::vector<long long int>> modes;

  long long int lastrefid = 0;
  long long int lastid = 0;
  if (chunk.size() > 0)
  {
    lastrefid = chunk[0].refID;
    lastid = chunk[0].ID;
  }

  // init edges
  for (unsigned int i = 0; i < chunk.size(); i += 1)
  {
    if (chunk[i].ID != lastid)
    {
      if (chunk[i].refID == lastrefid)
      {
        std::vector<long long int> edge{lastid, chunk[i].ID};
        modes.push_back(edge);
      }
    }
    lastrefid = chunk[i].refID;
    lastid = chunk[i].ID;
  }

  // no connected component was found
  if (modes.size() == 0)
  {
    return;
  }

  // init which modes are still active
  std::vector<bool> isActive(modes.size(), true);

  // expand connected component
  bool converged = false;
  while (!converged)
  {
    converged = true;
    for (unsigned int i = 0; i < modes.size(); i += 1)
    {
      if (isActive[i])
      {
        for (unsigned int j = 0; j < modes.size(); j += 1)
        {
          if (isActive[j] && i != j)
          {
            bool intersection = false;
            for (long long int jX : modes[j])
            {
              if (intersection)
              {
                break;
              }
              for (long long int iX : modes[i])
              {
                if (jX == iX)
                {
                  intersection = true;
                  converged = false;
                  break;
                }
              }
            }
            if (intersection)
            {
              isActive[j] = false;
              for (long long int jX : modes[j])
              {
                if (std::find(modes[i].begin(), modes[i].end(),
                              jX) == modes[i].end())
                {
                  modes[i].push_back(jX);
                }
              }
            }
          }
        }
      }
    }
  }

  for (unsigned int i = 0; i < modes.size(); i += 1)
  {
    if (isActive[i])
    {
      nameclusteroffset++;
      for (long long int iX : modes[i])
      {
        nameclusters.push_back(NameCluster(nameclusteroffset + maxcount, iX));
      }
    }
  }
}

void Otter::LocHandler::mergeNandW()
{
  stxxl::VECTOR_GENERATOR<NodeCoord>::result::bufreader_type readerN(nodes);
  stxxl::VECTOR_GENERATOR<WayRefs>::result::bufreader_type readerW(wayrefs);
  waynodes.reserve(wayrefs.size());
  stxxl::VECTOR_GENERATOR<WayCoord>::result::bufwriter_type writer(waynodes);
  while (!readerN.empty() && !readerW.empty())
  {
    if ((*readerN).ID == (*readerW).refID)
    {
      writer << WayCoord((*readerW).ID, (*readerW).order,
                         (*readerN).lon, (*readerN).lat, (*readerW).nameID, (*readerW).hastag);
      ++readerW;
    }
    else
    {
      ++readerN;
    }
  }
  writer.finish();
}

void Otter::LocHandler::writeways()
{
  // do a buffered read
  stxxl::VECTOR_GENERATOR<WayCoord>::result::bufreader_type reader(waynodes);
  // collect the wayrefs, then output
  std::vector<double> lons;
  std::vector<double> lats;
  bool lasthastag = false;
  long long int lastID = 0;
  if (waynodes.size() > 0)
  {
    lastID = (*reader).ID;
    lasthastag = (*reader).hastag;
  }
  else
  {
    std::cerr << "Vector size of waynodes is 0 while writing ways\n";
  }
  bool isPolygon = false;

  std::string precat = "";
  // computation of approx maxsize
  /*
   * mxs(subject) +
   * mxs(ID): approx with http://osmstats.neis-one.org/?item=elements +
   * 2* mxs(delim) +
   * mxs(geo) +
   * mxs(hasGeo) +
   * mxs(coordlist) +
   * mxs(wktsuffix) = ?
   *
   */

  /*
   * 8 + 12 + 2* 1 + 4 + 11 + 17 + (12 + n * 21) == 66 + f(n)
   *
   */

  std::string precatmid = conf.delim;
  precatmid += conf.geo;
  precatmid += conf.hasGeometry;
  precatmid += conf.delim;
  precatmid += "\"";

  synccountway = 0;

  {

    open();

    while (!reader.empty())
    {
      if ((*reader).ID != lastID || synccountway == waynodes.size() - 1)
      {
        if (synccountway == waynodes.size() - 1)
        {
          lons.push_back((*reader).lon);
          lats.push_back((*reader).lat);
        }
        precat.reserve(66 + 21 * lons.size());
        precat += conf.subjPrefixWay;
        precat += std::to_string(lastID);
        precat += precatmid;

        std::string wktstring = "";

        // process coords
        // check if the way is closed
        if (lons.size() > 2)
        {
          if (lons[0] == lons[lons.size() - 1] &&
              lats[0] == lats[lats.size() - 1])
          {
            wktstring += "POLYGON((";
            isPolygon = true;
          }
          else
          {
            wktstring += "LINESTRING(";
            isPolygon = false;
          }
        }
        else
        {
          wktstring += "LINESTRING(";
          isPolygon = false;
        }
        for (unsigned int i = 0; i < lons.size(); i += 1)
        {
          if (i > 0)
          {
            wktstring += ", ";
          }
          wktstring += std::to_string(lons[i]);
          wktstring += " ";
          wktstring += std::to_string(lats[i]);
        }
        if (isPolygon)
        {
          wktstring += ")";
        }
        precat += wktstring;

        wktstring += ")";
        // convert wktstring to polygon or segment and check if in rtree/dag
        if (lasthastag)
        {
          if (!isPolygon)
          {
            Linestring lines;
            bg::read_wkt(wktstring, lines);
            if (lons.size() > 1)
            {
              linestringwithin(lines, std::to_string(lastID));
            }
          }
          else
          {
            Polygon poly;
            bg::read_wkt(wktstring, poly);
            if (lons.size() > 1)
            {
              polywithin(poly, std::to_string(lastID));
            }
          }
        }
        if (!conf.onlystats)
        {
          precat += conf.wktSuffix;
          precat += conf.delim;
          precat += conf.triplend;
          outstream[0] << precat;
        }
        precat.clear();
        lons.clear();
        lats.clear();
        lastID = (*reader).ID;
        lasthastag = (*reader).nameID > 0 ? true : false;
      }
      lons.push_back((*reader).lon);
      lats.push_back((*reader).lat);
      ++reader;
      synccountway++;
    }
    close();
  }
}

void Otter::LocHandler::writemultipolygonrels()
{

  // init ofstream and filtering stream
  open();
  stxxl::VECTOR_GENERATOR<RelCoord>::result::bufreader_type reader(multipolygoncoords);

  std::vector<RelCoord> relnode;
  std::vector<std::vector<RelCoord>> outer;
  std::vector<std::vector<RelCoord>> inner;

  while (!reader.empty())
  {
    long long int currentid = (*reader).ID;
    int currentorder = (*reader).order;
    std::vector<RelCoord> relway;
    while (!reader.empty())
    {

      // advance reader until new id
      if ((*reader).ID != currentid)
      {
        if (relway.size() > 0)
        {
          if (relway[0].role > '1')
          {
            outer.push_back(relway);
          }
          if (relway[0].role == '0')
          {
            inner.push_back(relway);
          }
        }
        relway.clear();
        break;
      }
      if ((*reader).order != currentorder)
      {
        if (relway.size() > 0)
        {
          if (relway[0].role > '1')
          {
            outer.push_back(relway);
          }
          if (relway[0].role == '0')
          {
            inner.push_back(relway);
          }
        }
        relway.clear();
        currentorder = (*reader).order;
      }
      if ((*reader).type == 'n')
      {
        relnode.push_back((*reader));
      }
      else if ((*reader).type == 'w')
      {
        relway.push_back((*reader));
      }
      ++reader;
    }

    // processes wholerel
    writesinglerel(relnode, outer, inner, currentid);
    countwrittenmultipolygons++;
    relnode.clear();
    outer.clear();
    inner.clear();
  }
  close();
}

void Otter::LocHandler::writenonmultipolygonrels()
{

  // init ofstream and filtering stream
  open();
  stxxl::VECTOR_GENERATOR<RelCoord>::result::bufreader_type reader(nonmultipolygoncoords);

  std::vector<RelCoord> relnode;
  std::vector<std::vector<RelCoord>> outer;
  std::vector<std::vector<RelCoord>> inner;

  while (!reader.empty())
  {
    long long int currentid = (*reader).ID;
    int currentorder = (*reader).order;
    std::vector<RelCoord> relway;
    while (!reader.empty())
    {

      // advance reader until new id
      if ((*reader).ID != currentid)
      {
        if (relway.size() > 0)
        {
          if (relway[0].role > '1')
          {
            outer.push_back(relway);
          }
          if (relway[0].role == '0')
          {
            inner.push_back(relway);
          }
        }
        relway.clear();
        break;
      }
      if ((*reader).order != currentorder)
      {
        if (relway.size() > 0)
        {
          if (relway[0].role > '1')
          {
            outer.push_back(relway);
          }
          if (relway[0].role == '0')
          {
            inner.push_back(relway);
          }
        }
        relway.clear();
        currentorder = (*reader).order;
      }
      if ((*reader).type == 'n')
      {
        relnode.push_back((*reader));
      }
      else if ((*reader).type == 'w')
      {
        relway.push_back((*reader));
      }
      ++reader;
    }

    if (reader.empty())
    {
      if (relway[0].role > '1')
      {
        outer.push_back(relway);
      }
      if (relway[0].role == '0')
      {
        inner.push_back(relway);
      }
    }

    // processes wholerel

    writesinglerel(relnode, outer, inner, currentid);
    countwrittennonmultipolygons++;
    relnode.clear();
    outer.clear();
    inner.clear();
  }
  close();
}

/*inline*/ void Otter::LocHandler::writesinglerel(std::vector<RelCoord> &relnode,
                                           std::vector<std::vector<RelCoord>> &outer,
                                           std::vector<std::vector<RelCoord>> &inner, long long int idofrel)
{

  bool hastag = false;
  if (relnode.size() > 0)
  {
    if (relnode[0].hastag)
    {
      hastag = true;
    }
  }
  else
  {
    if (outer.size() > 0)
    {
      if (outer[0].size() > 0)
      {
        if (outer[0][0].hastag != 0)
        {
          hastag = true;
        }
      }
    }
  }

  std::vector<std::string> wktelements;
  std::string geogroup = conf.subjPrefixRelation;
  geogroup += std::to_string(idofrel);
  geogroup += conf.delim;
  geogroup += conf.geo;
  geogroup += conf.hasGeometry;
  geogroup += conf.delim;
  int sumsize = 0;
  for (RelCoord rc : relnode)
  {
    std::string wktpoint = conf.point;
    wktpoint += std::to_string(rc.lon);
    wktpoint += conf.delim;
    wktpoint += std::to_string(rc.lat);
    wktpoint += ")";
    wktelements.push_back(wktpoint);
    sumsize += wktpoint.size();
  }

  // role values:
  //   0 is inner
  //   1 is subarea
  //   2 is outer
  //   3 is empty
  //   4 is anything else

  // compute modes from roles: 2 & 3
  //   assume there is only 1 mode first, if not all outers
  //   are consumed, then generalize
  std::vector<std::vector<RelCoord>> outermodes;
  if (outer.size() > 0)
  {
    outermodes.push_back(outer[0]);
    outer.erase(outer.begin());
  }
  while (outer.size() > 0 && outermodes.size() > 0)
  {
    auto it = outer.begin();
    size_t lastsize = outermodes[outermodes.size() - 1].size();
    bool skip = false;

    while (it != outer.end() && !skip)
    {
      // check if outermodes[outermodes.size()-1] first or last
      //  contains (*it) with same lat lon
      if (outermodes[outermodes.size() - 1][0].lon == (*it)[0].lon &&
          outermodes[outermodes.size() - 1][0].lat == (*it)[0].lat)
      {
        // prepend in reverse order
        outermodes[outermodes.size() - 1].insert(outermodes[outermodes.size() - 1].begin(),
                                                 (*it).rbegin(), (*it).rend() - 1);

        it = outer.erase(it);
      }
      else if (outermodes[outermodes.size() - 1][0].lon ==
                   (*it)[(*it).size() - 1].lon &&
               outermodes[outermodes.size() - 1][0].lat ==
                   (*it)[(*it).size() - 1].lat)
      {
        // prepend
        outermodes[outermodes.size() - 1].insert(outermodes[outermodes.size() - 1].begin(),
                                                 (*it).begin(), (*it).end() - 1);

        it = outer.erase(it);
      }
      else if (outermodes[outermodes.size() - 1]
                         [outermodes[outermodes.size() - 1].size() - 1]
                             .lon == (*it)[0].lon &&
               outermodes[outermodes.size() - 1]
                         [outermodes[outermodes.size() - 1].size() - 1]
                             .lat == (*it)[0].lat)
      {
        // append

        outermodes[outermodes.size() - 1].insert(outermodes[outermodes.size() - 1].end(),
                                                 (*it).begin() + 1, (*it).end());

        it = outer.erase(it);
      }
      else if (outermodes[outermodes.size() - 1]
                         [outermodes[outermodes.size() - 1].size() - 1]
                             .lon ==
                   (*it)[(*it).size() - 1].lon &&
               outermodes[outermodes.size() - 1]
                         [outermodes[outermodes.size() - 1].size() - 1]
                             .lat ==
                   (*it)[(*it).size() - 1].lat)
      {
        // append in reverse order

        outermodes[outermodes.size() - 1].insert(outermodes[outermodes.size() - 1].end(),
                                                 (*it).rbegin() + 1, (*it).rend());
        it = outer.erase(it);
      }
      else
      {
        ++it;
      }
    }
    // stabilized?
    if (lastsize == outermodes[outermodes.size() - 1].size())
    {
      // all outers have been processed
      if (outer.size() == 0)
      {
        break;
      }
      else
      {
        outermodes.push_back(outer[0]);
        outer.erase(outer.begin());
      }
    }
  }

  std::vector<std::vector<RelCoord>> innermodes;
  if (inner.size() > 0)
  {
    innermodes.push_back(inner[0]);
    inner.erase(inner.begin());
  }
  while (inner.size() > 0 && innermodes.size() > 0)
  {
    auto it = inner.begin();
    size_t lastsize = innermodes[innermodes.size() - 1].size();
    bool skip = false;

    while (it != inner.end() && !skip)
    {
      // check if innermodes[innermodes.size()-1] first or last
      //  contains (*it) with same lat lon
      if (innermodes[innermodes.size() - 1][0].lon == (*it)[0].lon &&
          innermodes[innermodes.size() - 1][0].lat == (*it)[0].lat)
      {
        // prepend in reverse order
        innermodes[innermodes.size() - 1].insert(innermodes[innermodes.size() - 1].begin(),
                                                 (*it).rbegin(), (*it).rend() - 1);
        it = inner.erase(it);
      }
      else if (innermodes[innermodes.size() - 1][0].lon ==
                   (*it)[(*it).size() - 1].lon &&
               innermodes[innermodes.size() - 1][0].lat ==
                   (*it)[(*it).size() - 1].lat)
      {
        // prepend
        innermodes[innermodes.size() - 1].insert(innermodes[innermodes.size() - 1].begin(),
                                                 (*it).begin(), (*it).end() - 1);
        it = inner.erase(it);
      }
      else if (innermodes[innermodes.size() - 1]
                         [innermodes[innermodes.size() - 1].size() - 1]
                             .lon == (*it)[0].lon &&
               innermodes[innermodes.size() - 1]
                         [innermodes[innermodes.size() - 1].size() - 1]
                             .lat == (*it)[0].lat)
      {
        // append

        innermodes[innermodes.size() - 1].insert(innermodes[innermodes.size() - 1].end(),
                                                 (*it).begin() + 1, (*it).end());

        it = inner.erase(it);
      }
      else if (innermodes[innermodes.size() - 1]
                         [innermodes[innermodes.size() - 1].size() - 1]
                             .lon ==
                   (*it)[(*it).size() - 1].lon &&
               innermodes[innermodes.size() - 1]
                         [innermodes[innermodes.size() - 1].size() - 1]
                             .lat ==
                   (*it)[(*it).size() - 1].lat)
      {
        // append in reverse order

        innermodes[innermodes.size() - 1].insert(innermodes[innermodes.size() - 1].end(),
                                                 (*it).rbegin() + 1, (*it).rend());

        it = inner.erase(it);
      }
      else
      {
        ++it;
      }
    }
    // stabilized?
    if (lastsize == innermodes[innermodes.size() - 1].size())
    {
      // all inners have been processed
      if (inner.size() == 0)
      {
        break;
      }
      else
      {
        innermodes.push_back(inner[0]);
        inner.erase(inner.begin());
      }
    }
  }

  // detect which are polygons
  std::vector<std::vector<RelCoord>> outerpoly;
  std::vector<std::vector<RelCoord>> innerpoly;
  auto op = outermodes.begin();
  auto ip = innermodes.begin();
  while (op != outermodes.end())
  {
    if ((*op).size() > 3)
    {
      if ((*op)[0].lon == (*op)[(*op).size() - 1].lon &&
          (*op)[0].lat == (*op)[(*op).size() - 1].lat)
      {
        outerpoly.push_back((*op));
        op = outermodes.erase(op);
        continue;
      }
    }
    ++op;
  }

  while (ip != innermodes.end())
  {
    if ((*ip).size() > 3)
    {
      if ((*ip)[0].lon == (*ip)[(*ip).size() - 1].lon &&
          (*ip)[0].lat == (*ip)[(*ip).size() - 1].lat)
      {
        innerpoly.push_back((*ip));
        ip = innermodes.erase(ip);
        continue;
      }
    }
    ++ip;
  }
  // assign innerpolys to outerpolys
  auto opi = outerpoly.begin();
  auto ipi = innerpoly.begin();
  while (opi != outerpoly.end())
  {
    bool changed = false;
    //
    std::string innerstring = "";
    while (ipi != innerpoly.end())
    {
      if (polycontainspoly((*opi),
                           (*ipi)))
      {
        changed = true;
        // process iit and oit

        bool firstiteration = true;
        innerstring += ",(";
        firstiteration = true;
        for (RelCoord rc : (*ipi))
        {
          if (!firstiteration)
          {
            innerstring += ",";
          }
          firstiteration = false;
          innerstring += std::to_string(rc.lon);
          innerstring += conf.delim;
          innerstring += std::to_string(rc.lat);
        }
        ipi = innerpoly.erase(ipi);
        innerstring += ")";

        // break;
      }
      else
      {
        ++ipi;
      }
    }
    if (!changed)
    {
      ++opi;
    }
    else
    {
      std::string polystring = conf.polygon;
      polystring.reserve(polystring.size() +
                         (*opi).size() * 27 +
                         innerstring.size());
      bool firstiteration = true;
      for (RelCoord rc : (*opi))
      {
        if (!firstiteration)
        {
          polystring += ",";
        }
        firstiteration = false;
        polystring += std::to_string(rc.lon);
        polystring += conf.delim;
        polystring += std::to_string(rc.lat);
      }
      polystring += ")";
      polystring += innerstring;
      polystring += ")";

      wktelements.push_back(polystring);
      sumsize += polystring.size();
      opi = outerpoly.erase(opi);
    }
  }
  // evaluate the rest of innermodes and outermodes
  for (std::vector<RelCoord> singlemode : outermodes)
  {
    std::string polystring = conf.linestring;
    polystring.reserve(polystring.size() +
                       singlemode.size() * 27);
    bool firstiteration = true;
    for (RelCoord singlem : singlemode)
    {
      if (!firstiteration)
      {
        polystring += ",";
      }
      firstiteration = false;
      polystring += std::to_string(singlem.lon);
      polystring += conf.delim;
      polystring += std::to_string(singlem.lat);
    }
    polystring += ")";
    wktelements.push_back(polystring);
    sumsize += polystring.size();
  }
  for (std::vector<RelCoord> singlemode : outerpoly)
  {
    std::string polystring = conf.polygon;
    polystring.reserve(polystring.size() +
                       singlemode.size() * 27);
    bool firstiteration = true;
    for (RelCoord singlem : singlemode)
    {
      if (!firstiteration)
      {
        polystring += ",";
      }
      firstiteration = false;
      polystring += std::to_string(singlem.lon);
      polystring += conf.delim;
      polystring += std::to_string(singlem.lat);
    }
    polystring += "))";
    wktelements.push_back(polystring);
    sumsize += polystring.size();
  }
  geogroup += conf.geometrycollection;
  geogroup.reserve(geogroup.size() + sumsize + 2 * wktelements.size());
  bool firstrun = true;

  for (std::string readme : wktelements)
  {
    if (!firstrun)
    {
      geogroup += ",";
    }
    firstrun = false;
    geogroup += readme;
  }

  if (hastag)
  {
    // check within-relation for all wktelements
    std::vector<Point> gpoints;
    std::vector<Linestring> glines;
    std::vector<Polygon> gpolys;
    for (std::string readme : wktelements)
    {
      if (readme.size() > 2)
      {
        // POINT POLY OR LINESTRING
        if (readme[2] == 'I')
        {
          Point current;
          bg::read_wkt(readme, current);
          if (bg::is_valid(current))
          {
            gpoints.push_back(current);
          }
          else
          {
            bg::correct(current);
            if (bg::is_valid(current))
            {
              gpoints.push_back(current);
            }
          }
        }
        if (readme[2] == 'L')
        {
          Polygon current;
          bg::read_wkt(readme, current);
          if (bg::is_valid(current))
          {
            gpolys.push_back(current);
          }
          else
          {
            bg::correct(current);
            if (bg::is_valid(current))
            {
              gpolys.push_back(current);
            }
          }
        }
        if (readme[2] == 'N')
        {
          Linestring current;
          bg::read_wkt(readme, current);
          if (bg::is_valid(current))
          {
            glines.push_back(current);
          }
          else
          {
            bg::correct(current);
            if (bg::is_valid(current))
            {
              glines.push_back(current);
            }
          }
        }
      }
    }
    geocollectionwithin(gpoints, glines, gpolys, std::to_string(idofrel));
  }

  if (!conf.onlystats)
  {
    geogroup += conf.wktsuffixgc;
    outstream[0] << geogroup;
  }
}

// check if inner is inside outer
bool Otter::LocHandler::polycontainspoly(const std::vector<RelCoord> &outer, const std::vector<RelCoord> &inner)
{

  // heuristic: compute max and min of lat and lon for both
  // and compare with inner
  double omaxlon, omaxlat, ominlon, ominlat = 0;
  double imaxlon, imaxlat, iminlon, iminlat = 0;
  if (outer.size() > 0)
  {
    omaxlon = outer[0].lon;
    ominlon = outer[0].lon;
    omaxlat = outer[0].lat;
    ominlat = outer[0].lat;
  }
  if (inner.size() > 0)
  {
    imaxlon = inner[0].lon;
    iminlon = inner[0].lon;
    imaxlat = inner[0].lat;
    iminlat = inner[0].lat;
  }
  for (RelCoord curr : outer)
  {
    if (curr.lon > omaxlon)
    {
      omaxlon = curr.lon;
    }
    if (curr.lon < ominlon)
    {
      ominlon = curr.lon;
    }
    if (curr.lat > omaxlat)
    {
      omaxlat = curr.lat;
    }
    if (curr.lat < ominlat)
    {
      ominlat = curr.lat;
    }
  }
  for (RelCoord curr : inner)
  {
    if (curr.lon > imaxlon)
    {
      imaxlon = curr.lon;
    }
    if (curr.lon < iminlon)
    {
      iminlon = curr.lon;
    }
    if (curr.lat > imaxlat)
    {
      imaxlat = curr.lat;
    }
    if (curr.lat < iminlat)
    {
      iminlat = curr.lat;
    }
  }

  if (omaxlon > imaxlon &&
      omaxlat > imaxlat &&
      ominlon < iminlon &&
      ominlat < iminlat)
  {
    // check if within
    namespace bg = boost::geometry;
    typedef bg::model::point<double, 2, bg::cs::cartesian> point_t;
    typedef bg::model::polygon<point_t> polygon_t;
    polygon_t outerpoly;
    for (const RelCoord current : outer)
    {
      bg::append(outerpoly.outer(),
                 point_t(current.lon, current.lat));
    }
    polygon_t innerpoly;
    for (const RelCoord current : inner)
    {
      bg::append(innerpoly.outer(),
                 point_t(current.lon, current.lat));
    }
    return (boost::geometry::within(innerpoly, outerpoly));
  }
  else
  {
    return false;
  }

  return false;
}

inline void Otter::LocHandler::tagsToTriplesTTL(const std::vector<Tagpair> &tags, std::string subject)
{
  // use subject as common prefix of the tags
  subject += conf.delim;
  subject += conf.predPrefixDefault;
  // result = result + subject + conf.delim;
  for (const Tagpair &tag : tags)
  {
    std::string key(tag.k);
    std::string value(tag.v);
    key = url_encode(key);
    if (key.find("wiki") != std::string::npos)
    {
      if (key.find("wikidata") != std::string::npos && value.at(0) == 'Q' && key.find("note") == std::string::npos)
      {
        // split by ";" and use each object
        std::string trimmed = replaceAll(value, " ", "");
        for (const std::string &each : cppsplit(trimmed, ";"))
        {
          std::string tostream = subject;
          tostream.reserve(subject.length() + key.length() +
                           conf.delim.length() + conf.objPrefixWikidata.length() +
                           each.length() + conf.delim.length() +
                           conf.triplend.length());

          tostream += key;
          tostream += conf.delim;
          tostream += conf.objPrefixWikidata;
          tostream += each;
          tostream += conf.triplend;
          if (!conf.onlystats)
          {
            outstream[0] << tostream;
          }
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
            if (!conf.onlystats)
            {
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
            if (!conf.onlystats)
            {
              outstream[0] << tostream;
            }
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
          if (!conf.onlystats)
          {
            outstream[0] << tostream;
          }
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
        if (!conf.onlystats)
        {
          outstream[0] << tostream;
        }
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
      if (!conf.onlystats)
      {
        outstream[0] << tostream;
      }
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
      if (!conf.onlystats)
      {
        outstream[0] << tostream;
      }
    }
  }
}

std::vector<long long int> Otter::LocHandler::findsucc(long long int source)
{
  std::vector<long long int> sucs;
  findsuccrec(source, sucs);
  // sort and unique sucs
  std::sort(sucs.begin(), sucs.end());

  size_t sucsize = sucs.size();
  while (true)
  {
    auto last = std::unique(sucs.begin(), sucs.end());
    // v now holds {1 2 1 3 4 5 4 x x x}, where 'x' is indeterminate
    sucs.erase(last, sucs.end());
    if (sucsize == sucs.size())
    {
      break;
    }
    sucsize = sucs.size();
  }

  return sucs;
}

void Otter::LocHandler::findsuccrec(long long int source, std::vector<long long int> &sucs)
{

  if (DAG.contains(source))
  {
    for (long long int child : DAG[source])
    {
      sucs.push_back(child);
    }
    for (long long int child : DAG[source])
    {
      findsuccrec(child, sucs);
    }
  }
}

void Otter::LocHandler::linestringwithin(Linestring lines, std::string IDstring)
{
  Box box;
  boost::geometry::envelope(lines, box);
  std::vector<long long int> skipset;

  // rtree query
  std::vector<Boundpoly> result;
  rtree.query(bg::index::intersects(box), std::back_inserter(result));
  // sort packingmembers
  std::sort(result.begin(), result.end(),
            [this](Boundpoly &a, Boundpoly &b)
            {
              return std::get<5>(a) < std::get<5>(b);
            });

  {
    for (const Boundpoly &res : result)
    {
      if (std::find(skipset.begin(), skipset.end(), std::get<4>(res)) != skipset.end())
      {
        continue;
      }
      // check if in underestimation
      if (bg::intersects(lines, std::get<3>(res)) && bg::num_points(std::get<3>(res)) > 0)
      {
        countearlyaccept++;

        std::vector<std::string> printme;

        // check successor set via DAG
        std::vector<long long int> succs = findsucc(std::get<4>(res));
        succs.push_back(std::get<4>(res));
        for (const long long int succ : succs)
        {
          // print succ if not in skipset
          if (std::find(skipset.begin(), skipset.end(), succ) == skipset.end())
          {
            // print relation: seg inside succ

            printme.push_back(conf.subjPrefixNode);
            printme.push_back(IDstring);
            printme.push_back(conf.ispartofmiddlepart);
            printme.push_back(conf.subjPrefixRelation);
            printme.push_back(std::to_string(succ));
            printme.push_back(conf.triplend);

            printme.push_back(conf.subjPrefixRelation);
            printme.push_back(std::to_string(succ));
            printme.push_back(conf.ispartofmiddlepartreverse);
            printme.push_back(conf.subjPrefixNode);
            printme.push_back(IDstring);
            printme.push_back(conf.triplend);

            printme.push_back(conf.subjPrefixNode);
            printme.push_back(IDstring);
            printme.push_back(conf.intersectsmiddlepart);
            printme.push_back(conf.subjPrefixRelation);
            printme.push_back(std::to_string(succ));
            printme.push_back(conf.triplend);

            printme.push_back(conf.subjPrefixRelation);
            printme.push_back(std::to_string(succ));
            printme.push_back(conf.intersectsmiddlepart);
            printme.push_back(conf.subjPrefixNode);
            printme.push_back(IDstring);
            printme.push_back(conf.triplend);
          }
        }
        if (!conf.onlystats && printme.size() > 0)
        {
          outstream[0] << std::accumulate(printme.begin(), printme.end(), std::string{});
        }
        skipset.insert(skipset.end(), succs.begin(), succs.end());

        // write set of relations
      }
      else
      {
        // check if in overestimation
        if (bg::covered_by(lines, std::get<2>(res)) || bg::num_points(std::get<2>(res)) == 0)
        {
          // check if in actual
          countregular++;

          if (bg::covered_by(lines, std::get<1>(res)))
          {

            std::vector<std::string> printme;

            // check successor set via DAG
            std::vector<long long int> succs = findsucc(std::get<4>(res));
            succs.push_back(std::get<4>(res));
            for (const long long int succ : succs)
            {
              if (std::find(skipset.begin(), skipset.end(), succ) == skipset.end())
              {

                printme.push_back(conf.subjPrefixNode);
                printme.push_back(IDstring);
                printme.push_back(conf.ispartofmiddlepart);
                printme.push_back(conf.subjPrefixRelation);
                printme.push_back(std::to_string(succ));
                printme.push_back(conf.triplend);

                printme.push_back(conf.subjPrefixRelation);
                printme.push_back(std::to_string(succ));
                printme.push_back(conf.ispartofmiddlepartreverse);
                printme.push_back(conf.subjPrefixNode);
                printme.push_back(IDstring);
                printme.push_back(conf.triplend);

                printme.push_back(conf.subjPrefixNode);
                printme.push_back(IDstring);
                printme.push_back(conf.intersectsmiddlepart);
                printme.push_back(conf.subjPrefixRelation);
                printme.push_back(std::to_string(succ));
                printme.push_back(conf.triplend);

                printme.push_back(conf.subjPrefixRelation);
                printme.push_back(std::to_string(succ));
                printme.push_back(conf.intersectsmiddlepart);
                printme.push_back(conf.subjPrefixNode);
                printme.push_back(IDstring);
                printme.push_back(conf.triplend);
              }
            }
            if (!conf.onlystats && printme.size() > 0)
            {
              outstream[0] << std::accumulate(printme.begin(), printme.end(), std::string{});
            }
            skipset.insert(skipset.end(), succs.begin(), succs.end());
          }
        }
        else
        {
          countearlyreject++;
        }
      }
    }
  }

  // intersection
  {

    for (const Boundpoly &res : result)
    {
      if (std::find(skipset.begin(), skipset.end(), std::get<4>(res)) != skipset.end())
      {
        continue;
      }

      bool doesintersect = false;
      if (bg::intersects(lines, std::get<3>(res)))
      {
        doesintersect = true;
      }
      else
      {
        if (bg::intersects(lines, std::get<2>(res)))
        {
          // check if in actual
          if (bg::intersects(lines, std::get<1>(res)))
          {
            doesintersect = true;
          }
        }
      }

      if (!conf.onlystats && doesintersect)
      {
        std::string outputstring = conf.subjPrefixWay;
        outputstring.reserve(140);

        outputstring += IDstring;
        outputstring += conf.intersectsmiddlepart;
        outputstring += conf.subjPrefixRelation;
        outputstring += std::to_string(std::get<4>(res));
        outputstring += conf.triplend;

        outputstring += conf.subjPrefixRelation;
        outputstring += std::to_string(std::get<4>(res));
        outputstring += conf.intersectsmiddlepart;
        outputstring += conf.subjPrefixWay;
        outputstring += IDstring;
        outputstring += conf.triplend;

        if (!conf.onlystats)
        {
          outstream[0] << outputstring;
        }
      }
    }
  }
}

void Otter::LocHandler::polywithin(Polygon poly, std::string IDstring)
{
  // rtree query
  Box box;
  boost::geometry::envelope(poly, box);
  std::vector<long long int> skipset;
  // rtree query
  std::vector<Boundpoly> result;
  rtree.query(bg::index::intersects(box), std::back_inserter(result));
  // sort packingmembers
  std::sort(result.begin(), result.end(),
            [this](Boundpoly &a, Boundpoly &b)
            {
              return std::get<5>(a) < std::get<5>(b);
            });

  {
    auto it = result.begin();
    while (it != result.end())
    {
      if (std::find(skipset.begin(), skipset.end(), std::get<4>(*it)) != skipset.end())
      {
        ++it;
        continue;
      }
      // check if in underestimation
      if (bg::intersects(poly, std::get<3>(*it)) && bg::num_points(std::get<3>(*it)) > 0)
      {
        countearlyaccept++;
        // check successor set via DAG
        std::vector<long long int> succs = findsucc(std::get<4>(*it));
        succs.push_back(std::get<4>(*it));
        for (long long int succ : succs)
        {
          // print succ if not in skipset
          if (std::find(skipset.begin(), skipset.end(), succ) == skipset.end())
          {
            // print relation: seg inside succ
            std::string outputstring = conf.subjPrefixWay;
            outputstring.reserve(280);

            outputstring += IDstring;
            outputstring += conf.ispartofmiddlepart;
            outputstring += conf.subjPrefixRelation;
            outputstring += std::to_string(succ);
            outputstring += conf.triplend;

            // inverse relation
            outputstring += conf.subjPrefixRelation;
            outputstring += std::to_string(succ);
            outputstring += conf.ispartofmiddlepartreverse;
            outputstring += conf.subjPrefixWay;
            outputstring += IDstring;
            outputstring += conf.triplend;

            outputstring += conf.subjPrefixWay;
            outputstring += IDstring;
            outputstring += conf.intersectsmiddlepart;
            outputstring += conf.subjPrefixRelation;
            outputstring += std::to_string(succ);
            outputstring += conf.triplend;

            outputstring += conf.subjPrefixRelation;
            outputstring += std::to_string(succ);
            outputstring += conf.intersectsmiddlepart;
            outputstring += conf.subjPrefixWay;
            outputstring += IDstring;
            outputstring += conf.triplend;

            if (!conf.onlystats)
            {
              outstream[0] << outputstring;
            }
          }
        }

        skipset.insert(skipset.end(), succs.begin(), succs.end());

        // write set of relations
      }
      else
      {
        // check if in overestimation
        if (bg::covered_by(poly, std::get<2>(*it)) || bg::num_points(std::get<2>(*it)) == 0)
        {
          // check if in actual
          countregular++;
          if (bg::covered_by(poly, std::get<1>(*it)))
          {

            if (conf.onlystats)
            {
              ++it;
              continue;
            }

            // check successor set via DAG
            std::vector<long long int> succs = findsucc(std::get<4>(*it));
            succs.push_back(std::get<4>(*it));
            for (long long int succ : succs)
            {
              if (std::find(skipset.begin(), skipset.end(), succ) == skipset.end())
              {
                std::string outputstring = conf.subjPrefixWay;
                outputstring.reserve(280);

                outputstring += IDstring;
                outputstring += conf.ispartofmiddlepart;
                outputstring += conf.subjPrefixRelation;
                outputstring += std::to_string(succ);
                outputstring += conf.triplend;

                outputstring += conf.subjPrefixRelation;
                outputstring += std::to_string(succ);
                outputstring += conf.ispartofmiddlepartreverse;
                outputstring += conf.subjPrefixWay;
                outputstring += IDstring;
                outputstring += conf.triplend;

                outputstring += conf.subjPrefixWay;
                outputstring += IDstring;
                outputstring += conf.intersectsmiddlepart;
                outputstring += conf.subjPrefixRelation;
                outputstring += std::to_string(succ);
                outputstring += conf.triplend;

                outputstring += conf.subjPrefixRelation;
                outputstring += std::to_string(succ);
                outputstring += conf.intersectsmiddlepart;
                outputstring += conf.subjPrefixWay;
                outputstring += IDstring;
                outputstring += conf.triplend;

                if (!conf.onlystats)
                {
                  outstream[0] << outputstring;
                }
              }
            }
            skipset.insert(skipset.end(), succs.begin(), succs.end());
          }
        }
        else
        {
          countearlyreject++;
        }
      }
      ++it;
    }
  }

  {
    auto it = result.begin();
    while (it != result.end())
    {
      if (std::find(skipset.begin(), skipset.end(), std::get<4>(*it)) != skipset.end())
      {
        ++it;
        continue;
      }
      bool doesintersect = false;
      if (bg::intersects(poly, std::get<3>(*it)))
      {
        doesintersect = true;
      }
      else
      {
        if (bg::intersects(poly, std::get<2>(*it)))
        {
          // check if in actual
          if (bg::intersects(poly, std::get<1>(*it)))
          {
            doesintersect = true;
          }
        }
      }

      if (doesintersect)
      {
        std::string outputstring = conf.subjPrefixWay;
        outputstring.reserve(140);

        outputstring += IDstring;
        outputstring += conf.intersectsmiddlepart;
        outputstring += conf.subjPrefixRelation;
        outputstring += std::to_string(std::get<4>(*it));
        outputstring += conf.triplend;

        outputstring += conf.subjPrefixRelation;
        outputstring += std::to_string(std::get<4>(*it));
        outputstring += conf.intersectsmiddlepart;
        outputstring += conf.subjPrefixWay;
        outputstring += IDstring;
        outputstring += conf.triplend;

        if (!conf.onlystats)
        {
          outstream[0] << outputstring;
        }
      }
      ++it;
    }
  }
}

// check spatial relations for rel with ID
void Otter::LocHandler::geocollectionwithin(std::vector<Point> gpoints,
                                     std::vector<Linestring> glines,
                                     std::vector<Polygon> gpolys, std::string ID)
{

  bool earlyaccept = false;
  bool earlyreject = false;

  long long int currentID = std::stoll(ID);
  bool insideDAG = DAG.contains(currentID);

  // generate envelopes of all geometries and union them
  Multipolygon border;
  Multipolygon temppoly;
  // Multipolygon combined;
  if (gpoints.size() + glines.size() + gpolys.size() == 0)
  {
    return;
  }

  for (const Polygon &p : gpolys)
  {
    // add another polygon each iteration
    Box box;
    bg::envelope(p, box);
    Polygon current;
    bg::convert(box, current);
    bg::union_(border, current, temppoly);
    border = temppoly;
    boost::geometry::clear(temppoly);
  }
  for (const Linestring &l : glines)
  {
    // add another polygon each iteration
    Box box;
    bg::envelope(l, box);
    Polygon current;
    bg::convert(box, current);
    bg::union_(border, current, temppoly);
    border = temppoly;
    boost::geometry::clear(temppoly);
  }
  for (const Point &p : gpoints)
  {
    // add another polygon each iteration
    Box box;
    bg::envelope(p, box);
    Polygon current;
    bg::convert(box, current);
    bg::union_(border, current, temppoly);
    border = temppoly;
    boost::geometry::clear(temppoly);
  }
  Box totalbox;
  bg::envelope(border, totalbox);

  // query for totalbox
  std::vector<Boundpoly> result;
  rtree.query(bg::index::intersects(totalbox), std::back_inserter(result));

  // currentID

  auto removeme = result.begin();
  while (removeme != result.end())
  {
    if (std::get<4>((*removeme)) == currentID)
    {
      result.erase(removeme);
      break;
    }
    removeme++;
  }

  // sort packingmembers
  std::sort(result.begin(), result.end(),
            [this](Boundpoly &a, Boundpoly &b)
            {
              return std::get<5>(a) < std::get<5>(b);
            });
  auto it = result.begin();
  std::vector<long long int> skipset;
  if (insideDAG)
  {
    skipset.push_back(std::stoll(ID));
  }
  while (it != result.end())
  {

    if (std::find(skipset.begin(), skipset.end(), std::get<4>(*it)) != skipset.end())
    {
      ++it;
      continue;
    }

    bool abort = false;
    std::vector<bool> activepoints(gpoints.size(), true);
    std::vector<bool> activelines(glines.size(), true);
    std::vector<bool> activepolys(gpolys.size(), true);
    std::vector<bool> acceptedpoints(gpoints.size(), false);
    std::vector<bool> acceptedlines(glines.size(), false);
    std::vector<bool> acceptedpolys(gpolys.size(), false);
    for (unsigned int i = 0; i < 3; i += 1)
    {
      if (i == 0)
      {
        for (unsigned int j = 0; j < gpoints.size(); j += 1)
        {
          // check if in underestimation
          if (activepoints[j])
          {
            if (bg::covered_by(gpoints[j], std::get<3>(*it)) && bg::num_points(std::get<3>(*it)) > 0)
            {
              activepoints[j] = false;
              acceptedpoints[j] = true;
            }
          }
        }
        for (unsigned int j = 0; j < glines.size(); j += 1)
        {
          // check if in underestimation
          if (activelines[j])
          {
            if (bg::covered_by(glines[j], std::get<3>(*it)) && bg::num_points(std::get<3>(*it)) > 0)
            {
              // break;
              activelines[j] = false;
              acceptedlines[j] = true;
            }
          }
        }

        for (unsigned int j = 0; j < gpolys.size(); j += 1)
        {
          // check if in underestimation
          if (activepolys[j])
          {
            if (bg::covered_by(gpolys[j], std::get<3>(*it)) && bg::num_points(std::get<3>(*it)) > 0)
            {
              // break;
              activepolys[j] = false;
              acceptedpolys[j] = true;
            }
          }
        }
      }
      if (i == 1)
      {
        for (unsigned int j = 0; j < gpoints.size(); j += 1)
        {
          if (activepoints[j])
          {
            // check if not in overestimation
            if (!(bg::covered_by(gpoints[j], std::get<2>(*it)) || bg::num_points(std::get<2>(*it)) == 0))
            {
              activepoints[j] = false;
              acceptedpoints[j] = false;
              abort = true;
              break;
            }
          }
        }
        if (abort)
        {
          break;
        }
        for (unsigned int j = 0; j < glines.size(); j += 1)
        {
          if (activelines[j])
          {
            // check if not in overestimation
            if (!(bg::covered_by(glines[j], std::get<2>(*it)) || bg::num_points(std::get<2>(*it)) == 0))
            {
              activelines[j] = false;
              acceptedlines[j] = false;
              abort = true;
              break;
            }
          }
        }
        if (abort)
        {
          break;
        }
        for (unsigned int j = 0; j < gpolys.size(); j += 1)
        {
          if (activepolys[j])
          {
            // check if not in overestimation
            if (!(bg::covered_by(gpolys[j], std::get<2>(*it)) || bg::num_points(std::get<2>(*it)) == 0))
            {

              bool rejectme = true;

              if (insideDAG)
              {

                Multipolygon output;
                boost::geometry::difference(gpolys[j], std::get<2>(*it), output);
                // if area of diff is low and if inside union(outer+diff)

                float originarea = bg::area(gpolys[j]);
                float targetarea = bg::area(output);
                float division = targetarea / originarea;
                if (division < 0.00001 && division > 0)
                {
                  Multipolygon output2;
                  boost::geometry::union_(gpolys[j], std::get<2>(*it), output2);
                  if (bg::covered_by(gpolys[j], output2))
                  {
                    countpolytolerancesuccessful++;
                    rejectme = false;
                  }
                  else
                  {
                    countpolytolerancefailed++;
                  }
                }
              }

              if (rejectme)
              {
                countearlyreject++;
                activepolys[j] = false;
                acceptedpolys[j] = false;
                abort = true;
                break;
              }
            }
          }
        }
        if (abort)
        {
          break;
        }
      }
      if (i == 2)
      {
        if (abort)
        {
          break;
        }
        // check if in actual
        for (unsigned int j = 0; j < gpoints.size(); j += 1)
        {
          if (activepoints[j])
          {
            countregular++;
            if (bg::covered_by(gpoints[j], std::get<1>(*it)))
            {
              acceptedpoints[j] = true;
            }
            else
            {
              abort = true;
              break;
            }
          }
        }
        if (abort)
        {
          break;
        }
        // check if in actual
        for (unsigned int j = 0; j < glines.size(); j += 1)
        {
          if (activelines[j])
          {
            countregular++;
            if (bg::covered_by(glines[j], std::get<1>(*it)))
            {
              acceptedlines[j] = true;
            }
            else
            {
              abort = true;
              break;
            }
          }
        }

        if (abort)
        {
          break;
        }
        // check if in actual
        for (unsigned int j = 0; j < gpolys.size(); j += 1)
        {
          if (activepolys[j])
          {
            countregular++;
            if (bg::covered_by(gpolys[j], std::get<1>(*it)))
            {
              acceptedpolys[j] = true;
            }
            else
            {

              bool rejectme = true;

              if (insideDAG)
              {
                Multipolygon output;
                boost::geometry::difference(gpolys[j], std::get<1>(*it), output);
                // if area of diff is low and if inside union(outer+diff)

                float originarea = bg::area(gpolys[j]);
                float targetarea = bg::area(output);
                float division = targetarea / originarea;
                if (division < 0.00001 && division > 0)
                {
                  Multipolygon output2;
                  boost::geometry::union_(gpolys[j], std::get<1>(*it), output2);
                  if (bg::covered_by(gpolys[j], output2))
                  {
                    rejectme = false;
                    activepolys[j] = false;
                    acceptedpolys[j] = true;
                    countpolytolerancesuccessful++;
                  }
                  else
                  {
                    activepolys[j] = false;
                    acceptedpolys[j] = false;
                    countpolytolerancefailed++;
                  }
                }
              }

              if (rejectme)
              {
                abort = true;
                break;
              }
            }
          }
        }
      }
    }

    // validate acceptedvectors and so on
    bool onlyaccepted = true;
    if (!abort)
    {
      for (bool acc : acceptedpoints)
      {
        if (!acc)
        {
          onlyaccepted = false;
          break;
        }
      }
      for (bool acc : acceptedlines)
      {
        if (!acc)
        {
          onlyaccepted = false;
          break;
        }
      }
      for (bool acc : acceptedpolys)
      {
        if (!acc)
        {
          onlyaccepted = false;
          break;
        }
      }
      if (!conf.onlystats && onlyaccepted)
      {

        // check successor set via DAG
        std::vector<long long int> succs = findsucc(std::get<4>(*it));
        succs.push_back(std::get<4>(*it));
        for (long long int succ : succs)
        {
          if (std::find(skipset.begin(), skipset.end(), succ) == skipset.end())
          {
            // write
            std::string outputstring = conf.subjPrefixRelation;
            outputstring.reserve(280);
            outputstring += ID;
            outputstring += conf.ispartofmiddlepart;
            outputstring += conf.subjPrefixRelation;
            outputstring += std::to_string(succ);
            outputstring += conf.triplend;

            outputstring += conf.subjPrefixRelation;
            outputstring += std::to_string(succ);
            outputstring += conf.ispartofmiddlepartreverse;
            outputstring += conf.subjPrefixRelation;
            outputstring += ID;
            outputstring += conf.triplend;

            outputstring += conf.subjPrefixRelation;
            outputstring += ID;
            outputstring += conf.intersectsmiddlepart;
            outputstring += conf.subjPrefixRelation;
            outputstring += std::to_string(succ);
            outputstring += conf.triplend;

            outputstring += conf.subjPrefixRelation;
            outputstring += std::to_string(succ);
            outputstring += conf.intersectsmiddlepart;
            outputstring += conf.subjPrefixRelation;
            outputstring += ID;
            outputstring += conf.triplend;

            if (!conf.onlystats)
            {
              outstream[0] << outputstring;
            }
          }
        }
        skipset.insert(skipset.end(), succs.begin(), succs.end());
      }
    }
    else
    {
      // check intersection
      bool doesintersect = false;
      bool nonIntersectsWithOverestimation = true;
      for (Polygon p : gpolys)
      {
        if (bg::intersects(p, std::get<2>(*it)))
        {
          nonIntersectsWithOverestimation = false;
          break;
        }
      }

      for (Linestring p : glines)
      {
        if (!nonIntersectsWithOverestimation)
        {
          break;
        }

        if (bg::intersects(p, std::get<2>(*it)))
        {
          nonIntersectsWithOverestimation = false;
        }
      }
      for (Point p : gpoints)
      {
        if (!nonIntersectsWithOverestimation)
        {
          break;
        }
        if (bg::intersects(p, std::get<2>(*it)))
        {
          nonIntersectsWithOverestimation = false;
        }
      }

      if (!nonIntersectsWithOverestimation)
      {
        for (Polygon p : gpolys)
        {
          if (doesintersect)
          {
            break;
          }
          if (bg::intersects(p, std::get<3>(*it)))
          {
            doesintersect = true;
            break;
          }
        }

        for (Linestring p : glines)
        {
          if (doesintersect)
          {
            break;
          }

          if (bg::intersects(p, std::get<3>(*it)))
          {
            doesintersect = true;
          }
        }
        for (Point p : gpoints)
        {
          if (doesintersect)
          {
            break;
          }
          if (bg::intersects(p, std::get<3>(*it)))
          {
            doesintersect = true;
          }
        }

        for (Polygon p : gpolys)
        {
          if (doesintersect)
          {
            break;
          }
          if (bg::intersects(p, std::get<1>(*it)))
          {
            doesintersect = true;
            break;
          }
        }

        for (Linestring p : glines)
        {
          if (doesintersect)
          {
            break;
          }

          if (bg::intersects(p, std::get<1>(*it)))
          {
            doesintersect = true;
          }
        }
        for (Point p : gpoints)
        {
          if (doesintersect)
          {
            break;
          }
          if (bg::intersects(p, std::get<1>(*it)))
          {
            doesintersect = true;
          }
        }
      }

      if (!conf.onlystats && doesintersect)
      {

        // write intersection
        std::string outputstring = conf.subjPrefixRelation;
        outputstring.reserve(140);

        outputstring += ID;
        outputstring += conf.intersectsmiddlepart;
        outputstring += conf.subjPrefixRelation;
        outputstring += std::to_string(std::get<4>(*it));
        outputstring += conf.triplend;

        outputstring += conf.subjPrefixRelation;
        outputstring += std::to_string(std::get<4>(*it));
        outputstring += conf.intersectsmiddlepart;
        outputstring += conf.subjPrefixRelation;
        outputstring += ID;
        outputstring += conf.triplend;

        if (!conf.onlystats)
        {
          outstream[0] << outputstring;
        }
      }
    }
    ++it;
  }
}