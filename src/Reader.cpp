#include "Reader.h"

Otter::Reader::Reader(Configurator cfg)
{

  osmium::io::File input_file{cfg.datainpathV};

  if (!cfg.onlystats)
  { // first read: write tags and node coords

    Timer clock;

    osmium::io::Reader reader{input_file};
    print("----- Write tags and node coords -----");
    TagHandler handler(cfg);
    clock.tick("Printing tags");
    osmium::apply(reader, handler);
    clock.tock();
    handler.close();
    reader.close();
  }
  else
  {
    print("----- Skip tags and writing of node coords -----");
  }

  { // second read: write way coords
    Timer whole;
    whole.tick();

    Timer clock;

    osmium::io::Reader reader{input_file};
    // first read to count the stats
    print("----- Read for osm stats(count to reserve external memory) -----");
    clock.tick("Calculate stats of the osm elements");
    StatHandler sh;
    osmium::apply(reader, sh);
    clock.tock();
    print("nodecount: ", sh.nodes);
    print("waycount: ", sh.ways);
    print("relationcount: ", sh.relations);
    print("countRelRels: ", sh.countRelRels);
    print("countRelsNoRels: ", sh.countRelsNoRels);
    print("counttypemultipolygon: ", sh.counttypemultipolygon);
    print("counttypemultipolygonandrelrel: ", sh.counttypemultipolygonandrelrel);
    print("counttypemultipolygonandtypeboundary: ", sh.counttypemultipolygonandtypeboundary);
    print("counttypeboundary: ", sh.counttypeboundary);
    reader.close();

    // calculate relevant boundaries
    print("----- Compute boundaries  -----");
    Otter::BoundaryHandler bh;
    clock.tick("Pass for relations...");
    osmium::io::Reader relationreader{input_file, osmium::osm_entity_bits::relation};
    osmium::apply(relationreader, bh);
    relationreader.close();
    clock.tock();
    clock.tick("Sort usedways of boundary handler...");
    std::sort(bh.usedways.begin(), bh.usedways.end());
    clock.tock();

    clock.tick("Pass for ways...");

    osmium::io::Reader wayreader{input_file, osmium::osm_entity_bits::way};
    osmium::apply(wayreader, bh);
    wayreader.close();
    clock.tock();

    std::cout << "usednodes.size():" << bh.usednodes.size() << "\n";
    clock.tick("Sort usednodes of boundary handler...");
    std::sort(bh.usednodes.begin(), bh.usednodes.end());
    clock.tock();

    bh.usedways.clear();
    clock.tick("Pass for nodes...");
    osmium::io::Reader nodereader{input_file, osmium::osm_entity_bits::node};
    osmium::apply(nodereader, bh);
    nodereader.close();
    clock.tock();
    bh.usednodes.clear();
    // sort before merging
    clock.tick("Sort wayrefs by refID and nodecoords by ID...");
    std::sort(bh.nodecoords.begin(), bh.nodecoords.end(), nodeCmptr());
    std::sort(bh.wayrefs.begin(), bh.wayrefs.end(), wayCmptr());
    clock.tock();
    clock.tick("Merge wayrefs & nodecoords to waycoords...");
    bh.computewaycoords();
    clock.tock();
    bh.nodecoords.clear();
    bh.wayrefs.clear();
    clock.tick("Sort RelRefs by refID & waycoords by ID...");
    std::sort(bh.boundaries.begin(), bh.boundaries.end(), relrefbyrefid());
    std::sort(bh.waycoords.begin(), bh.waycoords.end(), wayCoordIDorder());
    clock.tock();
    clock.tick("Merge waycoords & boundaries to boundariesresolved...");
    bh.resolveboundaries();
    clock.tock();
    bh.boundaries.clear();
    bh.waycoords.clear();

    clock.tick("Sort RelCoords by ID and order");
    std::sort(bh.boundariesresolved.begin(), bh.boundariesresolved.end(), relcoordbyidordersuborder());
    clock.tock();

    clock.tick("Assemble polygons of boundaries...");
    bh.assemblegeometry();
    print("below: ", bh.nBelowThresh);
    print("above: ", bh.nAboveThresh);
    clock.tock();
    print("Number of assembled boundaries: ", bh.countBoundary);

    clock.tick("Create rtree with boundaries...");
    bh.genrtree();
    clock.tock();

    clock.tick("Sort packing members by area...");
    bh.sortpackingmembers();
    clock.tock();

    clock.tick("Initialize DAG from rtree...");
    bh.genDAG();
    clock.tock();

    print("Counter how many overestimations were computed:", bh.countOverestimate);
    print("Number of successful reparations when overestimating:", bh.countRepairOverestimateSuccess);
    print("Number of failed reparations when overestimating:", bh.countRepairOverestimateFail);
    print("sumrelativepointreductionratioOver:", bh.sumrelativepointreductionOver);
    print("numberrelativepointreductionOver:", bh.numberrelativepointreductionOver);
    print("");

    print("Counter how many underestimation were computed:", bh.countUnderestimate);
    print("Number of successful reparations when underestimating:", bh.countRepairUnderestimateSuccess);
    print("Number of failed reparations when underestimating:", bh.countRepairUnderestimateFail);
    print("sumrelativepointreductionratioUnder:", bh.sumrelativepointreductionUnder);
    print("numberrelativepointreductionUnder:", bh.numberrelativepointreductionUnder);
    print("");

    print("totalPointsBoundary:", bh.totalPointsBoundary);
    print("numberPointsBoundary:", bh.numberPointsBoundary);

    // use the stats to reserve the space
    osmium::io::Reader r{input_file};
    print("----- Storing osm elements  -----");
    Otter::LocHandler lh(cfg, sh);
    lh.DAG.swap(bh.DAG);
    std::cout << "lh.DAG.size():" << lh.DAG.size() << std::endl;
    lh.rtree.swap(bh.rtree);
    clock.tick("Reserve stxxl vectors for osm elements");
    lh.nodes.reserve(sh.nodes);
    lh.wayrefs.reserve(sh.ways);
    lh.multipolygonrefs.reserve(sh.counttypemultipolygon);
    lh.nonmultipolygonrefs.reserve(sh.relations - sh.counttypemultipolygon - sh.countRelRels);
    clock.tock();
    clock.tick("Storing coords of nodes, wayrefs, relrefs and within-relations for nodes");
    lh.open();
    osmium::apply(r, lh);
    lh.close();
    clock.tock();
    r.close();
    print("");
    print("countearlyaccept:", lh.countearlyaccept);
    print("countearlyreject:", lh.countearlyreject);
    print("countregular:", lh.countregular);

    {
      // sort nodes and wayrefs
      print("----- Process way geometry  -----");
      clock.tick("Sorting nodes and ways");
      lh.sortNodes();
      lh.sortWays();
      clock.tock();

      clock.tick("Merging nodes and ways to resolve noderefs");
      lh.mergeNandW();
      clock.tock();

      print("nodes.size(): ", lh.nodes.size());
      print("wayrefs.size(): ", lh.wayrefs.size());
      print("waynodes.size(): ", lh.waynodes.size());

      // compute nameclusters
      bool sortedwaynodes = false;
      {
        print("----- Compute ways, which depict a fragment of a way -----");
        clock.tick("Compute nameclusters(cluster wayfragements of same name)");
        // sort wayrefs by refID
        lh.sortWayrefsByRefID();
        // generate new way IDs with their linked refIds in nameclusters
        lh.findconnectedcomponents();
        // write tags of new ways
        lh.enablenameclusters();
        lh.armnamedclusters();
        osmium::io::Reader r{input_file};
        // init output stream
        lh.open();
        osmium::apply(r, lh);
        lh.close();
        r.close();
        print("Number of named clusters:", lh.countNameCluster);
        // write geometry of nameclusters
        clock.tick("Sorting waynodes");
        lh.sortWaynodes();
        clock.tock();
        sortedwaynodes = true;
        lh.writenameclustergeometry();
        clock.tock();
      }
      // clear after nameclusters, since wayrefs are used for nameclusters
      clock.tick("Clearing wayrefs");
      lh.wayrefs.clear();
      clock.tock();
      if (!sortedwaynodes)
      {
        clock.tick("Sorting waynodes");
        lh.sortWaynodes();
        clock.tock();
      }
      clock.tick("Printing geometry of ways");
      // open and close called inside writeways
      lh.writeways();
      clock.tock();
    }

    // write relations
    /*
     * the relations are stored in 3 container:
     *  set tagged with type = multipolygon(stxxl vector)
     *  set not tagged with type = multipolygon(stxxl vector)
     *  set of relations that has subrelations(map)
     */
    {
      print("----- Process relation geometry  -----");
      // loop over stored nodes and process those for the rels
      // nodes are sorted by ID
      // sort relRefs by refID
      print("count multipolygonrefs entries: ", lh.multipolygonrefs.size());
      print("count non-multipolygonrefs entries: ", lh.nonmultipolygonrefs.size());
      clock.tick("Sorting multipolygon refs");
      lh.sortmultiypolygonrefs();
      clock.tock();
      clock.tick("Resolve multipolygon-refs wrt. nodes");
      lh.resolvenodesinmultipolygons();
      clock.tock();
      {
        clock.tick("Sorting nonmultipolygon refs");
        lh.nonsortmultiypolygonrefs();
        clock.tock();
        clock.tick("Resolve nonmultipolygon-refs wrt. nodes");
        print("before node resolving nonmultipolygoncoords:", lh.nonmultipolygoncoords.size());
        lh.resolvenodesinnonmultipolygons();
        print("after node resolving nonmultipolygoncoords:", lh.nonmultipolygoncoords.size());
        clock.tock();
      }
      lh.nodes.clear(); // nodes vector was processed
      // ways are sorted by ID and order
      clock.tick("Resolve multipolygon-refs wrt. ways");
      lh.resolvewaysinmultipolygons();
      clock.tock();
      {
        clock.tick("Resolve nonmultipolygon-refs wrt. ways");
        lh.resolvewaysinnonmultipolygons();
        clock.tock();
      }
      lh.waynodes.clear();
      print("count multipolygoncoords entries: ", lh.multipolygoncoords.size());
      print("count non-multipolygoncoords entries: ", lh.nonmultipolygoncoords.size());

      // sort multipolygoncoords and nonmultipolygoncoords by
      // ID, order and suborder
      clock.tick("Sort multipolygon-relcoords by ID, order and suborder");
      lh.sortmultipolygon();
      clock.tock();
      clock.tick("Write geometry of rels with tag: type = multipolygon");
      lh.writemultipolygonrels();
      clock.tock();
      {
        clock.tick("Sort nonmultipolygon-relcoords by ID, order and suborder");
        lh.sortnonmultipolygon();
        clock.tock();
        clock.tick("Write geometry of rels that do not have tag: type = multipolygon");
        lh.writenonmultipolygonrels();
        clock.tock();
      }
      print("Number of written multipolygons:", lh.countwrittenmultipolygons);
      print("Number of written non-multipolygons:", lh.countwrittennonmultipolygons);
    }
    print("Number of early accepted:", lh.countearlyaccept);
    print("Number of early rejected:", lh.countearlyreject);
    print("Number of regular accepted/rejected:", lh.countregular);

    print("Count polygon-tolerance approach success:", lh.countpolytolerancesuccessful);
    print("Count polygon-tolerance approach fail:", lh.countpolytolerancefailed);

    lh.close();
    whole.tock("Net runtime: ");
  }
}
