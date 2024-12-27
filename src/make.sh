#!/bin/sh

bison --defines=parse.h -o parse.c parse.y
flex --header-file=scan.h -o scan.c scan.l

cc -o parse.o -c parse.c
cc -o scan.o -c scan.c
c++ -g -std=gnu++17 -o parser.o -c parser.cc
c++ -g -std=gnu++17 -o test_scan.o -c test_scan.cc
c++ -g -std=gnu++17 -o test_parse.o -c test_parse.cc
c++ -g -std=gnu++17 -o config.o -c config.cc
c++ -g -std=gnu++17 -o gf_ops.o -c gf_ops.cc

cc -g scan.o parse.o parser.o test_parse.o gf_ops.o config.o -o test_scan \
   -lstdc++ -Wall
cc -g scan.o parse.o parser.o test_parse.o gf_ops.o config.o -o test_parse \
   -lstdc++ -Wall


## C++17 is required for:
#   std::variant<>
#   std::filesystem::path
# g++ parser.cc -o parser -lstdc++ -ll -std=c++17
# cc parser.cc parse.c scan.c -o parser -lstdc++ -ll -std=c++17
