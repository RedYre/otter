#include "Configurator.h"
#include "Reader.h"

#include <string>
#include <iostream>

int main(int argc, char *argv[])
{
  // use args as the prefered user input
  Otter::Configurator cr(argc, argv);

  // pass configurator in order to provide the arg option values
  Otter::Reader rr(cr);

  return 0;
}