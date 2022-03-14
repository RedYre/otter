#include "StatHandler.h"

Otter::StatHandler::StatHandler()
{
}

// This callback is called by osmium::apply for each node in the data.
void Otter::StatHandler::node(const osmium::Node &node)
{
  nodes++;
}

// This callback is called by osmium::apply for each way in the data.
void Otter::StatHandler::way(const osmium::Way &way)
{
  ways++;
  if (maxway < way.id())
  {
    maxway = way.id();
  }
}

// This callback is called by osmium::apply for each relation in the data.
void Otter::StatHandler::relation(const osmium::Relation &relation)
{
  relations++;
  {
    bool relrel = false;
    bool istypemultipolygon = false;
    bool istypeboundary = false;
    if (relation.tags().has_tag("type", "boundary"))
    {
      istypeboundary = true;
      counttypeboundary++;
      if (relation.tags().has_tag("boundary", "administrative"))
      {
        countdoublcond++;
        if (relation.tags().has_key("admin_level"))
        {
          counttriplecond++;
          if (relation.tags().has_key("name"))
          {
            countquadracond++;
          }
        }
      }
    }
    if (relation.tags().has_tag("type", "multipolygon"))
    {
      istypemultipolygon = true;
      counttypemultipolygon++;
    }
    const osmium::RelationMemberList &rml = relation.members();
    for (const osmium::RelationMember &rm : rml)
    {
      if (rm.type() == osmium::item_type::relation)
      {
        relrel = true;
      }
    }
    if (relrel)
    {
      countRelRels++;
      if (istypemultipolygon)
      {
        counttypemultipolygonandrelrel++;
        if (istypeboundary)
        {
          counttypemultipolygonandtypeboundary++;
        }
      }
    }
    else
    {
      countRelsNoRels++;
    }
  }
}
