#include "BoundaryHandler.h"

Otter::BoundaryHandler::BoundaryHandler()
{
}

// This callback is called by osmium::apply for each node in the data.
void Otter::BoundaryHandler::node(const osmium::Node &node)
{
  if (std::binary_search(usednodes.begin(), usednodes.end(), node.id()))
  {
    nodecoords.push_back(
        NodeCoord(
            node.id(), node.location().lon(), node.location().lat()));
  }
}

// This callback is called by osmium::apply for each way in the data.
void Otter::BoundaryHandler::way(const osmium::Way &way)
{
  if (std::binary_search(usedways.begin(), usedways.end(), way.id()))
  {
    int order = 0;
    for (const osmium::NodeRef &nr : way.nodes())
    {
      usednodes.push_back(nr.ref());
      wayrefs.push_back(
          WayRefs(
              way.id(), order, nr.ref(), 0, false));
      order++;
    }
  }
}

// This callback is called by osmium::apply for each relation in the data.
void Otter::BoundaryHandler::relation(const osmium::Relation &relation)
{
  if (relation.tags().has_tag("type", "boundary"))
  {
    if (relation.tags().has_tag("boundary", "administrative"))
    {
      if (relation.tags().has_key("admin_level"))
      {
        if (relation.tags().has_key("name"))
        {
          std::vector<long long int> subs;
          bool isrelrel = false;
          for (const osmium::RelationMember &rm : relation.members())
          {
            if (rm.type() == osmium::item_type::relation)
            {
              if (strcmp(rm.role(), "subarea") == 0)
              {
                subs.push_back(rm.ref());
              }
              else
              {
                isrelrel = true;
              }
            }
          }
          if (!isrelrel)
          {
            int order = 0;
            // add to boundary
            bool hastag = relation.tags().size() > 0;
            for (const osmium::RelationMember &rm : relation.members())
            {
              if (rm.type() == osmium::item_type::way)
              {
                char rol = '4';
                if (strcmp(rm.role(), "inner") == 0)
                {
                  rol = '0';
                }
                if (strcmp(rm.role(), "subarea") == 0)
                {
                  rol = '1';
                }
                if (strcmp(rm.role(), "outer") == 0)
                {
                  rol = '2';
                }
                if (strcmp(rm.role(), "") == 0)
                {
                  rol = '3';
                }
                usedways.push_back(rm.ref());
                boundaries.push_back(
                    RelRefs(
                        relation.id(), order, 'w',
                        rm.ref(), 0, rol, hastag));
                order++;
              }
            }
          }
        }
      }
    }
  }
}

void Otter::BoundaryHandler::computewaycoords()
{
  // loop through wayrefs and nodecoords

  std::vector<NodeCoord>::iterator readerN = nodecoords.begin();
  std::vector<WayRefs>::iterator readerW = wayrefs.begin();
  waycoords.reserve(wayrefs.size());
  while (!(readerN == nodecoords.end()) && !(readerW == wayrefs.end()))
  {
    if ((*readerN).ID == (*readerW).refID)
    {
      waycoords.push_back(
          WayCoord(
              (*readerW).ID, (*readerW).order,
              (*readerN).lon, (*readerN).lat, 0, (*readerW).hastag));
      ++readerW;
    }
    else
    {
      ++readerN;
    }
  }
}

void Otter::BoundaryHandler::resolveboundaries()
{

  std::vector<WayCoord>::iterator readerW = (waycoords.begin());
  std::vector<RelRefs>::iterator readerR = (boundaries.begin());
  boundariesresolved.reserve(waycoords.size());

  std::vector<WayCoord> waymembers;
  while (!(readerW == waycoords.end()) && !(readerR == boundaries.end()))
  {
    if ((*readerW).ID == (*readerR).refID)
    {
      if ((*readerR).type == 'w')
      {
        long long int actid = (*readerW).ID;
        while (!(readerW == waycoords.end()))
        {
          if ((*readerW).ID != actid)
          {
            break;
          }
          waymembers.push_back((*readerW));
          ++readerW;
        }
        while (!(readerR == boundaries.end()))
        {
          if ((*readerR).refID != actid)
          {
            break;
          }
          // process waymembers with (*readerR)
          for (WayCoord wc : waymembers)
          {
            boundariesresolved.push_back(
                RelCoord(
                    (*readerR).ID, (*readerR).order, wc.order, 'w',
                    wc.lon, wc.lat, (*readerR).nameID, (*readerR).role, false));
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
}

void Otter::BoundaryHandler::assemblegeometry()
{

  std::vector<RelCoord>::iterator reader = (boundariesresolved.begin());

  std::vector<std::vector<RelCoord>> outer;
  std::vector<std::vector<RelCoord>> inner;

  while (!(reader == boundariesresolved.end()))
  {
    long long int currentid = (*reader).ID;
    int currentorder = (*reader).order;
    std::vector<RelCoord> relway;
    while (!(reader == boundariesresolved.end()))
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
      if ((*reader).type == 'w')
      {
        relway.push_back((*reader));
      }
      ++reader;
    }

    if (reader == boundariesresolved.end())
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

    genpoly(outer, inner, currentid);
    countBoundary++;

    outer.clear();
    inner.clear();
  }
}

Polygon Otter::BoundaryHandler::relcoordstopolygon(std::vector<RelCoord> omodes)
{
  Polygon res;
  // append RelCoords of omodes to res
  for (RelCoord rc : omodes)
  {
    bg::append(res, Point(rc.lon, rc.lat));
  }
  // in case winding direction isnt correct, or last element != first element
  bg::correct(res);
  return res;
}

// ___________________________________________________________
void Otter::BoundaryHandler::genpoly(/*std::vector<RelCoord>& relnode,*/
                              std::vector<std::vector<RelCoord>> &outer,
                              std::vector<std::vector<RelCoord>> &inner,
                              long long int relid)
{

  if (outer.size() > 3000)
  {
    nAboveThresh++;
    return;
  }
  else
  {
    nBelowThresh++;
  }

  // unchanged multipolygon of outers and assigned inners
  Multipolygon mpg;
  // simplified outers(underestimation, is contained by original)
  // with inner polygons cut out
  Multipolygon simpleinside;
  // simplified outers(overestimation, contains original)
  Multipolygon simpleoutside;

  // role values:
  //   0 is inner
  //   1 is subarea
  //   2 is outer
  //   3 is empty
  //   4 is anything else

  // compute modes from roles: 2 & 3
  std::vector<std::vector<RelCoord>> outermodes;
  if (outer.size() > 0)
  {
    // outermodes.push_back(outer[0]);
    outermodes.push_back((outer[0]));
    outer.erase(outer.begin());
  }
  else
  {
    return;
  }

  // connect all modes, that share nodes... consider correct appending
  // and prepending
  while (outer.size() > 0 && outermodes.size() > 0)
  {
    auto it = outer.begin();
    // used to compute "convergence"
    size_t lastsize = outermodes[outermodes.size() - 1].size();
    bool skip = false;

    // connecting run.. will be repeated until convergence
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

  std::vector<Polygon> innermpg;

  auto op = outermodes.begin();
  auto ip = innermodes.begin();
  while (op != outermodes.end())
  {
    if ((*op).size() > 3)
    {
      if ((*op)[0].lon == (*op)[(*op).size() - 1].lon &&
          (*op)[0].lat == (*op)[(*op).size() - 1].lat)
      {
        mpg.push_back(relcoordstopolygon((*op)));
        op = outermodes.erase(op);
        continue;
      }
    }
    ++op;
  }

  if (mpg.size() == 0)
  {
    return;
  }

  while (ip != innermodes.end())
  {
    if ((*ip).size() > 3)
    {
      if ((*ip)[0].lon == (*ip)[(*ip).size() - 1].lon &&
          (*ip)[0].lat == (*ip)[(*ip).size() - 1].lat)
      {
        innermpg.push_back(relcoordstopolygon((*ip)));
        bg::correct(innermpg[innermpg.size() - 1]);
        ip = innermodes.erase(ip);
        continue;
      }
    }
    ++ip;
  }

  bg::correct(mpg);

  // init index
  auto opi = mpg.begin();

  // bool simpleoutsidestillgood = true;

  // assign inner to outers
  while (opi != mpg.end())
  {

    // compute area
    float areainit = bg::area((*opi));

    // count how many polygons were added by reducepointsinner
    size_t countsimples = 0;
    size_t countsimplesbefore = simpleinside.size();
    // reduce points of (*opi) via score, fix via boost algorithms
    reappendlastpoint((*opi));

    bg::unique((*opi));
    bg::correct((*opi));

    simpleoutside.resize(simpleoutside.size() + 1);
    reducepointsouter((*opi), simpleoutside);

    // remove empty polygons
    auto it = simpleoutside.begin();
    while (it != simpleoutside.end())
    {
      if (bg::num_points(*it) == 0)
      {
        it = simpleoutside.erase(it);
      }
      else
      {
        ++it;
      }
    }

    reducepointsinner((*opi), simpleinside, areainit);
    countsimples = simpleinside.size() - countsimplesbefore;

    // check if inner is inside
    auto ipi = innermpg.begin();
    while (ipi != innermpg.end())
    {
      // break;
      // check polygon winding(clockwise or anti..)

      if (bg::covered_by(*ipi, *opi))
      {
        // assign inner to outer
        Polygon approxinner = (*ipi);
        bg::correct(approxinner);
        (*opi).inners().resize((*opi).inners().size() + 1);
        bg::append(*opi, approxinner.outer(), (*opi).inners().size() - 1);

        // simple version: just add inner
        for (size_t i = 0; i < countsimples; i += 1)
        {
          // check if disjoint, if not:
          if (!bg::disjoint(approxinner,
                            simpleinside[simpleinside.size() - 1 - i].outer()))
          {
            Multipolygon multi;
            boost::geometry::difference(simpleinside, approxinner, multi);
            simpleinside = multi;
            bg::correct(simpleinside);
          }
        }

        // done with this ipi, since we tried to process it
        ipi = innermpg.erase(ipi);
      }
      else
      {
        ++ipi;
      }
    }
    ++opi;
  }

  bg::correct(mpg);

  if (mpg.size() > 0)
  {

    Box bx;
    boost::geometry::envelope(mpg, bx);

    bg::strategy::area::spherical<> sph_strategy(6371008.8);

    // approx area
    float area = bg::area(mpg, sph_strategy);

    // set (Boundpoly) adminbound

    Multipolygon output;
    boost::geometry::union_(mpg, simpleoutside, output);

    // increment stats for relativepointreduction
    if (boost::geometry::num_points(simpleinside) > 0)
    {
      sumrelativepointreductionUnder +=
          (1.0f * boost::geometry::num_points(simpleinside)) / boost::geometry::num_points(mpg);
      ++numberrelativepointreductionUnder;
    }

    if (boost::geometry::num_points(output) > 0)
    {
      sumrelativepointreductionOver +=
          (1.0f * boost::geometry::num_points(output)) / boost::geometry::num_points(mpg);
      ++numberrelativepointreductionOver;
    }

    packingmembers.push_back(std::make_tuple(bx, mpg, output, simpleinside, relid, area));
  }

  totalPointsBoundary += boost::geometry::num_points(mpg);
  numberPointsBoundary++;
}

// check if inner is inside outer
bool Otter::BoundaryHandler::polycontainspoly(const std::vector<RelCoord> &outer, const std::vector<RelCoord> &inner)
{
  // check if within

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

  return false;
}

void Otter::BoundaryHandler::genrtree()
{
  boost::geometry::index::rtree<Boundpoly,
                                boost::geometry::index::quadratic<16>>
      temptree(packingmembers.begin(), packingmembers.end());
  rtree.swap(temptree);
}

// use for underestimation
void Otter::BoundaryHandler::simplifiedpolygon(std::vector<RelCoord> inputPoints,
                                        Multipolygon &outputPoints, size_t l, size_t r, double eps, bool inside)
{
  // r cannot be smaller than l
  if (r < l)
  {
    std::cerr << "r < l for Simplified polygon \n";
    exit(1);
  }

  if (l == r)
  {
    bg::append(outputPoints[outputPoints.size() - 1].outer(),
               Point(inputPoints[l].lon, inputPoints[l].lat));
    return;
  }

  if (l + 1 == r)
  {
    bg::append(outputPoints[outputPoints.size() - 1].outer(),
               Point(inputPoints[l].lon, inputPoints[l].lat));
    bg::append(outputPoints[outputPoints.size() - 1].outer(),
               Point(inputPoints[r].lon, inputPoints[r].lat));
    return;
  }

  // l < m < r
  size_t m;
  float maxdist = -1;
  size_t mleft = l;
  size_t mright = l;
  double maxdistleft = 0;
  double maxdistright = 0;
  bool simplified = false;

  RelCoord L = inputPoints[l];
  RelCoord R = inputPoints[r];

  if (L.lon == R.lon &&
      L.lat == R.lat)
  {
    return;
  }

  // compute most left and most right perpendicDistance
  for (unsigned int k = l + 1; k <= r - 1; k += 1)
  {
    double dist = perpendiculardistance(L, R, inputPoints[k]);
    if (dist < 0 && -dist > maxdistleft)
    {
      mleft = k;
      maxdistleft = -dist;
    }

    if (dist > 0 && dist > maxdistright)
    {
      mright = k;
      maxdistright = dist;
    }
  }

  // compute either inner poly or outer poly with reduced points
  if (inside)
  {
    simplified = (maxdistleft == 0 && maxdistright <= eps);
    m = maxdistleft > 0 ? mleft : mright;
  }
  else
  {
    simplified = (maxdistright == 0 && maxdistleft <= eps);
    m = maxdistright > 0 ? mright : mleft;
  }

  if (simplified)
  {
    bg::append(outputPoints[outputPoints.size() - 1].outer(),
               Point(L.lon, L.lat));
    bg::append(outputPoints[outputPoints.size() - 1].outer(),
               Point(R.lon, R.lat));
    return;
  }

  simplifiedpolygon(inputPoints,
                    outputPoints, l, m, eps, inside);
  simplifiedpolygon(inputPoints,
                    outputPoints, m + 1, r, eps, inside);
}

// use for overestimation
void Otter::BoundaryHandler::simplifiedpolygon(Polygon inputPoints,
                                        Multipolygon &outputPoints, size_t l, size_t r, double eps, bool inside)
{
  // r cannot be smaller than l
  if (r < l)
  {
    std::cerr << "r < l for Simplified polygon \n";
    exit(1);
  }
  if (l == r)
  {
    bg::append(outputPoints[outputPoints.size() - 1].outer(),
               inputPoints.outer()[l]);
    return;
  }
  if (l + 1 == r)
  {
    bg::append(outputPoints[outputPoints.size() - 1].outer(),
               inputPoints.outer()[l]);
    bg::append(outputPoints[outputPoints.size() - 1].outer(),
               inputPoints.outer()[r]);
    return;
  }
  // l < m < r
  size_t m;
  float maxdist = -1;
  size_t mleft = l;
  size_t mright = l;
  double maxdistleft = 0;
  double maxdistright = 0;
  bool simplified = false;

  Point L = inputPoints.outer()[l];
  Point R = inputPoints.outer()[r];

  if (bg::get<0>(L) == bg::get<0>(R) &&
      bg::get<1>(L) == bg::get<1>(L))
  {
    return;
  }
  // compute most left and most right perpendicDistance
  for (unsigned int k = l + 1; k <= r - 1; k += 1)
  {
    double dist = perpendiculardistance(L, R, inputPoints.outer()[k]);
    if (dist < 0 && -dist > maxdistleft)
    {
      mleft = k;
      maxdistleft = -dist;
    }

    if (dist > 0 && dist > maxdistright)
    {
      mright = k;
      maxdistright = dist;
    }
  }
  // compute either inner poly or outer poly with reduced points
  if (inside)
  {
    simplified = (maxdistleft == 0 && maxdistright <= eps);
    m = maxdistleft > 0 ? mleft : mright;
  }
  else
  {
    simplified = (maxdistright == 0 && maxdistleft <= eps);
    m = maxdistright > 0 ? mright : mleft;
  }
  if (simplified)
  {
    bg::append(outputPoints[outputPoints.size() - 1].outer(),
               L);
    bg::append(outputPoints[outputPoints.size() - 1].outer(),
               R);
    return;
  }
  simplifiedpolygon(inputPoints,
                    outputPoints, l, m, eps, inside);
  simplifiedpolygon(inputPoints,
                    outputPoints, m + 1, r, eps, inside);
}

double Otter::BoundaryHandler::perpendiculardistance(RelCoord A,
                                              RelCoord B, RelCoord C)
{

  if (A.lon == B.lon &&
      A.lat == B.lat)
  {
    std::cerr << "A == B while perpendiculardistance... thats undefined... \n";
    exit(1);
  }
  double distAB =
      std::sqrt(
          (A.lon - B.lon) * (A.lon - B.lon) +
          (A.lat - B.lat) * (A.lat - B.lat));
  double areatriangle =
      (B.lat - A.lat) * (A.lon - C.lon) -
      (B.lon - A.lon) * (A.lat - C.lat);
  return areatriangle / distAB;
}

double Otter::BoundaryHandler::perpendiculardistance(Point A,
                                              Point B, Point C)
{

  if (bg::get<0>(A) == bg::get<0>(B) &&
      bg::get<1>(A) == bg::get<1>(B))
  {
    std::cerr << "A == B while perpendiculardistance... thats undefined... \n";
    exit(1);
  }
  double distAB =
      std::sqrt(
          (bg::get<0>(A) - bg::get<0>(B)) * (bg::get<0>(A) - bg::get<0>(B)) +
          (bg::get<1>(A) - bg::get<1>(B)) * (bg::get<1>(A) - bg::get<1>(B)));
  double areatriangle =
      (bg::get<1>(B) - bg::get<1>(A)) * (bg::get<0>(A) - bg::get<0>(C)) -
      (bg::get<0>(B) - bg::get<0>(A)) * (bg::get<1>(A) - bg::get<1>(C));
  return areatriangle / distAB;
}

void Otter::BoundaryHandler::sortpackingmembers()
{
  // sort packingmembers
  std::sort(packingmembers.begin(), packingmembers.end(),
            [this](Boundpoly &a, Boundpoly &b)
            {
              return std::get<5>(a) < std::get<5>(b);
            });
}

void Otter::BoundaryHandler::genDAG()
{

  // set capacity
  DAG.reserve(packingmembers.size());

  // packingmembers is sorted by area: low areas first
  // loop over packingmembers and create DAGNodes based on rtree
  for (unsigned int i = 0; i < packingmembers.size(); i += 1)
  {
    // query rtree for within
    std::vector<Boundpoly> result;
    rtree.query(bg::index::contains(std::get<0>(packingmembers[i])), std::back_inserter(result));
    // sort packingmembers
    std::sort(result.begin(), result.end(),
              [this](Boundpoly &a, Boundpoly &b)
              {
                return std::get<5>(a) < std::get<5>(b);
              });

    // init skipset
    std::vector<long long int> skipset;

    // check for all checking member

    for (Boundpoly res : result)
    {

      if (std::get<4>(res) == std::get<4>(packingmembers[i]))
      {
        continue;
      }

      if (std::find(skipset.begin(), skipset.end(), std::get<4>(res)) != skipset.end())
      {
        continue;
      }

      // Boundpoly tuple index:
      // [1] : actual multipolygon
      // [2] : overestimation mp
      // [3] : underestimation mp

      // first check if not in overestimation => rejected

      // check if within underestimation => accept
      if (bg::covered_by(std::get<1>(packingmembers[i]), std::get<3>(res)) && bg::num_points(std::get<3>(res)) > 0)
      {
        // accept edge to DAG
        DAG[std::get<4>(packingmembers[i])].push_back(std::get<4>(res));

        // add res and its DAG successors to skipset
        std::vector<long long int> addtoskip = findsucc(std::get<4>(res));
        skipset.insert(skipset.end(), addtoskip.begin(), addtoskip.end());
      }
      else
      {
        // check if inside overestimation
        if (bg::covered_by(std::get<1>(packingmembers[i]), std::get<2>(res)) || bg::num_points(std::get<2>(res)) == 0)
        {
          // accept edge since its in the original mp
          if (bg::covered_by(std::get<1>(packingmembers[i]), std::get<1>(res)))
          {

            if (std::get<5>(res) == std::get<5>(packingmembers[i]))
            {
              continue;
            }

            DAG[std::get<4>(packingmembers[i])].push_back(std::get<4>(res));

            // add res and its DAG successors to skipset
            std::vector<long long int> addtoskip = findsucc(std::get<4>(res));
            skipset.insert(skipset.end(), addtoskip.begin(), addtoskip.end());
          }
        }
      }
    }
  }
}




void Otter::BoundaryHandler::reducepointsinner(Polygon in, Multipolygon &out, float areainit)
{
  // compute reduced that are contained in the former polygon
  size_t points = bg::num_points(in);
  if (points < 30)
  {
    // dont reduce low point polygons
    return;
  }
  countUnderestimate++;

  // boundaries
  double minepsilon = 0.00000001;
  double maxepsilon = 0.013;

  float bestscoresofar = -1000000;
  float lastscore = -1000000;

  Multipolygon bestpolysofar;

  float minepsscore = -1000000;
  float maxepsscore = -1000000;

  int count = 6;
  double epsdelta = (maxepsilon - minepsilon) / count;

  while (minepsilon < maxepsilon)
  {
    Multipolygon currentpoly;
    currentpoly.resize(currentpoly.size() + 1);
    simplifiedpolygon(in, currentpoly, 0, in.outer().size() - 2, minepsilon, false);
    reappendlastpoint(currentpoly);
    boost::geometry::unique(currentpoly);
    std::string failure = "";
    size_t currentpoints = boost::geometry::num_points(currentpoly);
    float score = scoreofpoly(points, areainit,
                              currentpoints, boost::geometry::area(currentpoly));

    bool valid = boost::geometry::is_valid(currentpoly, failure);

    if (currentpoints < points)
    {
      if (currentpoints < 7)
      {
        break;
      }
      if (score >= bestscoresofar)
      {
        bestscoresofar = score;
        bestpolysofar = currentpoly;
      }
      if (score >= lastscore)
      {
        lastscore = score;
      }
      else
      {
        // local optimization
        break;
      }
    }

    minepsilon = minepsilon + epsdelta;
  }

  bool addbestpoly = false;
  std::string f = "";
  if (boost::geometry::is_valid(bestpolysofar, f))
  {

    // work with addbestpoly
    addbestpoly = true;
  }
  else
  {
    if (boost::geometry::num_points(bestpolysofar) > 10)
    {
      resolveintersection(bestpolysofar[0]);

      reappendlastpoint(bestpolysofar);
      boost::geometry::unique(bestpolysofar);
      reappendlastpoint(bestpolysofar);

      std::string failure = "";
      if (!boost::geometry::is_valid(bestpolysofar, failure))
      {
        std::vector<Polygon> output;
        boost::geometry::intersection(bestpolysofar, in, output);
        bestpolysofar.erase(bestpolysofar.end() - 1);
        // compare reduced area with original area to validate
        for (Polygon mp : output)
        {
          if (boost::geometry::is_valid(mp))
          {
            addbestpoly = true;
            bestpolysofar.resize(bestpolysofar.size() + 1);
            bestpolysofar.push_back(mp);
          }
        }
        if (addbestpoly)
        {
          countRepairUnderestimateSuccess++;
        }
        else
        {
          countRepairUnderestimateFail++;
        }
      }
      else
      {
        addbestpoly = true;
      }
    }
  }

  if (addbestpoly)
  {
    // add to out
    for (Polygon p : bestpolysofar)
    {
      // remove zero sized extring
      if (p.outer().size() > 0)
      {
        if (p.inners().size() > 0)
        {
          auto itbesti = boost::begin(p.inners());
          while (itbesti != boost::end(p.inners()))
          {
            if ((*itbesti).size() == 0)
            {
              // erase
              itbesti = p.inners().erase(itbesti);
              continue;
            }
            ++itbesti;
          }
        }
        out.push_back(p);
      }
    }
  }
}

void Otter::BoundaryHandler::reappendlastpoint(Multipolygon &poly)
{
  if (poly.size() == 0 || bg::num_points(poly) == 0)
  {
    return;
  }
  auto it = boost::begin(boost::geometry::exterior_ring(poly[0]));
  auto it2 = boost::end(boost::geometry::exterior_ring(poly[0]));
  --it2;
  if (bg::get<0>(*it) != bg::get<0>(*it2) || bg::get<1>(*it) != bg::get<1>(*it2))
  {
    bg::append(poly[0].outer(), (*it));
  }
}

void Otter::BoundaryHandler::reappendlastpoint(Polygon &poly)
{
  if (bg::num_points(poly) == 0)
  {
    return;
  }
  auto it = boost::begin(boost::geometry::exterior_ring(poly));
  auto it2 = boost::end(boost::geometry::exterior_ring(poly));
  --it2;
  if (bg::get<0>(*it) != bg::get<0>(*it2) || bg::get<1>(*it) != bg::get<1>(*it2))
  {
    bg::append(poly.outer(), (*it));
  }
}

float Otter::BoundaryHandler::scoreofpoly(size_t maxpoint, float areainit,
                                   size_t currentpoint, float currentarea)
{
  float termA = (1.0f - (1.0f * currentpoint / (1.0f * maxpoint)));
  float termB = (currentarea / areainit);
  return 0.40 * termA + 0.60 * termB;
}

void Otter::BoundaryHandler::resolveintersection(Polygon &in)
{

  // is it neccesary?
  bg::unique(in.outer());

  // {
  // compute self turns
  typedef boost::geometry::detail::overlay::turn_info<Point, boost::geometry::segment_ratio<double>> TurnInfoType;
  bg::detail::no_rescale_policy robust_policy;
  bg::detail::self_get_turn_points::no_interrupt_policy interrupt_policy;
  std::vector<TurnInfoType> turns;

  typename bg::strategy::intersection::services::default_strategy<typename bg::cs_tag<Polygon>::type>::type strategy;
  boost::geometry::self_turns<boost::geometry::detail::overlay::assign_null_policy>(in.outer(), strategy, robust_policy, turns, interrupt_policy);
  // store segments
  std::vector<std::vector<size_t>> segs;

  for (TurnInfoType ti : turns)
  {
    std::vector<size_t> seg;

    if (ti.operations[0].seg_id.segment_index > ti.operations[1].seg_id.segment_index)
    {
      seg.push_back(ti.operations[1].seg_id.segment_index);
      seg.push_back(ti.operations[0].seg_id.segment_index);
    }
    else
    {
      seg.push_back(ti.operations[0].seg_id.segment_index);
      seg.push_back(ti.operations[1].seg_id.segment_index);
    }
    segs.push_back(seg);
  }

  if (segs.size() == 0)
  {
    return;
  }

  std::vector<bool> isactive;
  for (unsigned int i = 0; i < segs.size(); i += 1)
  {
    isactive.push_back(true);
  }

  // converged if no reduction of the segment modes can be found
  size_t lastsize = 0;
  size_t thissize = segs.size();
  while (lastsize != thissize)
  {
    lastsize = thissize;
    for (unsigned int i = 0; i < segs.size(); i += 1)
    {
      if (isactive[i])
      {
        for (unsigned int j = 0; j < segs.size(); j += 1)
        {
          if (i != j && isactive[j])
          {
            // connect segments if they share nodes
            std::vector<size_t> incommon;
            std::set_intersection(
                segs[i].begin(), segs[i].end(),
                segs[j].begin(), segs[j].end(), std::back_inserter(incommon));
            if (incommon.size() > 0)
            {
              std::vector<size_t> unionset;
              std::set_union(
                  segs[i].begin(), segs[i].end(),
                  segs[j].begin(), segs[j].end(),
                  std::back_inserter(unionset));
              isactive[j] = false;
              segs[i] = unionset;
              thissize--;
            }
          }
        }
      }
    }
  }

  // compute cluster with min(paths)
  std::vector<size_t> rangestart;
  std::vector<size_t> rangeend;
  for (unsigned int i = 0; i < segs.size(); i += 1)
  {
    if (segs[i].size() > 1 && isactive[i])
    {
      size_t segvalstart = 0;
      size_t segvalend = 0;
      size_t minpath = 1000000000000;
      for (unsigned int j = 0; j < segs[i].size(); j += 1)
      {
        size_t indexleft = (j + segs[i].size() - 1) % segs[i].size();
        size_t nodedistance =
            ((segs[i][indexleft] + in.outer().size()) - segs[i][j]) % in.outer().size();
        if (minpath > nodedistance)
        {
          minpath = nodedistance;
          segvalstart = segs[i][j];
          segvalend = segs[i][indexleft];
        }
      }
      rangestart.push_back(segvalstart);
      rangeend.push_back(segvalend);
    }
  }

  // set bools of the computed ranges
  std::vector<bool> activenodes(in.outer().size(), true);

  for (unsigned int i = 0; i < rangestart.size(); i += 1)
  {

    if (rangeend[i] < rangestart[i])
    {
      size_t distance = ((rangeend[i] + in.outer().size() - rangestart[i])) % in.outer().size();
      for (unsigned int j = 0; j < distance; j += 1)
      {
        activenodes[(in.outer().size() - rangestart[i] + j) % in.outer().size()] = false;
      }
    }
    else
    {
      size_t distance = ((rangeend[i] - rangestart[i]));
      for (unsigned int j = 0; j < distance; j += 1)
      {
        activenodes[(in.outer().size() - rangestart[i] + j) % in.outer().size()] = false;
      }
    }
  }

  // collect index of elements that shall be deleted
  std::vector<size_t> deletelist;
  for (unsigned int i = 0; i < activenodes.size(); i += 1)
  {
    if (!activenodes[i])
    {
      // offset due to polygon winding direction
      deletelist.push_back(in.outer().size() - 1 - i);
    }
  }

  for (unsigned int i = in.outer().size() - 1; i >= 0; i -= 1)
  {
    if (deletelist.size() == 0)
    {
      break;
    }
    if (i == deletelist[0])
    {
      in.outer().erase(in.outer().begin() + i);
      deletelist.erase(deletelist.begin());
    }
  }
}

void Otter::BoundaryHandler::reducepointsouter(Polygon input, Multipolygon &output)
{
  // check if the pointcount of the input is too enough, no need to simplify
  if (bg::num_points(input) == 0)
  {
    return;
  }
  if (bg::num_points(input) < 30)
  {
    Polygon temp;
    bg::convert(bg::return_envelope<Box>(input.outer()), temp);
    output.push_back(temp);
    return;
  }
  bool success = false;

  countOverestimate++;

  // correction not needed, input was corrected before
  double minepsilon = 0.013; // 0.0001; // 0.013;

  simplifiedpolygon(input, output, 0, input.outer().size() - 2, minepsilon, true);

  if (bg::num_points(output[output.size() - 1]) > 0)
  {
    reappendlastpoint(output[output.size() - 1]);
  }
  boost::geometry::unique(output[output.size() - 1]);
  if (bg::num_points(output[output.size() - 1]) > 0)
  {
    reappendlastpoint(output[output.size() - 1]);
  }

  if (!bg::covered_by(input, output[output.size() - 1]) || !bg::is_valid(output[output.size() - 1]))
  {

    bg::correct(output[output.size() - 1]);
    bg::correct(input);

    // union
    Multipolygon unionpoly;
    boost::geometry::union_(input, output[output.size() - 1], unionpoly);
    bg::correct(unionpoly);
    if (bg::num_points(unionpoly) < 0.5 * bg::num_points(output[output.size() - 1]))
    {
      // prob failed
      // resolve intersection
      resolveintersection(output[output.size() - 1]);

      reappendlastpoint(output[output.size() - 1]);
      boost::geometry::unique(output[output.size() - 1]);
      reappendlastpoint(output[output.size() - 1]);

      Polygon tempres = output[output.size() - 1];
      output[output.size() - 1].clear();
      simplifiedpolygon(tempres, output, 0, tempres.outer().size() - 2, minepsilon * 6, true);
      bg::correct(output[output.size() - 1]);

      // check for valid?

      if (bg::is_valid(output))
      {
        success = true;
      }
    }
    else
    {
      if (bg::is_valid(unionpoly))
      {
        success = true;
        countRepairOverestimateSuccess++;
      }
      else
      {
        countRepairOverestimateFail++;
      }
      output.erase(output.begin() + output.size() - 1);

      for (Polygon pu : unionpoly)
      {
        output.push_back(pu);
      }
    }
  }
  else
  {
    success = true;
  }

  if (!success)
  {
    // add box as simplest overestimation
    Polygon temp;
    bg::convert(bg::return_envelope<Box>(input.outer()), temp);
    output.clear();
    output.push_back(temp);
  }
}

std::vector<long long int> Otter::BoundaryHandler::findsucc(long long int source)
{
  std::vector<long long int> sucs;
  findsuccrec(source, sucs);
  // sort and unique sucs
  std::sort(sucs.begin(), sucs.end());

  size_t sucsize = sucs.size();
  while (true)
  {
    auto last = std::unique(sucs.begin(), sucs.end());
    sucs.erase(last, sucs.end());
    if (sucsize == sucs.size())
    {
      break;
    }
    sucsize = sucs.size();
  }

  return sucs;
}

void Otter::BoundaryHandler::findsuccrec(long long int source, std::vector<long long int> &sucs)
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