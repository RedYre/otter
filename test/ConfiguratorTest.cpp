#include "gtest/gtest.h"
#include "Configurator.h"

TEST(Configurator, fileExists)
{
  Otter::Configurator cfg;
  EXPECT_TRUE(cfg.fileExists("/container/bin/Makefile"));
}

TEST(Configurator, containsArg)
{
  {
    char *arguments[] = {
        (char *)("./otter"),
        (char *)("-osmpath"),
        (char *)("../mountme/bremen-latest.osm.pbf"),
        (char *)("-ttlpath"),
        (char *)("../mountme/bremen.gz")};

    Otter::Configurator cfg(5, arguments);
    EXPECT_TRUE(!cfg.containsArg(""));
    EXPECT_TRUE(!cfg.containsArg("./otter"));
    EXPECT_TRUE(cfg.containsArg("osmpath"));
    EXPECT_TRUE(cfg.containsArg("ttlpath"));
    EXPECT_TRUE(!cfg.containsArg("help"));
  }
}
