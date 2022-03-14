#include "gtest/gtest.h"
#include "ArgumentOption.h"

TEST(ArgumentOptionTest, getAndSet)
{
  Otter::ArgumentOption ao("ttlpath", "output path of ttl file", 2, Otter::ArgumentOption::Type::STRING);
  EXPECT_TRUE(ao.getoption() == "ttlpath");
  EXPECT_TRUE(ao.getdescr() == "output path of ttl file");
  EXPECT_TRUE(ao.getvals() == 2);
  EXPECT_TRUE(ao.gettype() == Otter::ArgumentOption::Type::STRING);

  ao.addparamstring("addme");
  EXPECT_TRUE(ao.getparamstring()[0] == "addme");

  ao.addparamint(99);
  EXPECT_TRUE(ao.getparamint()[0] == 99);

  ao.setmatched(true);
  EXPECT_TRUE(ao.getmatched());
}
