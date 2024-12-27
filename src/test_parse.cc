
#include <iostream>
#include <stdio.h>
#include <string.h>

#include "config.h"

extern int gf_debug;

int main(int argc, char *argv[])
{
  try {

    gf_debug = 1;

    std::unique_ptr<Config> conf;

    try {
      GFParser P;
      conf.reset(P.parse_file(argc>=2 ? argv[1] : NULL));
    }catch(std::exception& e){
      std::cerr<<"Parse error: "<<e.what()<<"\n";
      return 1;
    }

    std::cerr<<"Generic AST:\n";
    std::cerr << *conf;
    std::cerr<<"GF:\n";
    GFPrint(std::cout, *conf);

    fprintf(stderr, "Done\n");
    return 0;
  }catch(std::exception& e){
    std::cerr<<"Error: "<<e.what()<<"\n";
    return 1;
  }
}
