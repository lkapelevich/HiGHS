#ifndef __SRC_LIB_FEASIBILITY_HPP__
#define __SRC_LIB_FEASIBILITY_HPP__

#include <cstdlib>

struct CrashSolution {
   std::vector<unsigned int> active;
   std::vector<unsigned int> inactive;
	std::vector<BasisStatus> rowstatus;
   Vector primal;
   Vector rowact;

   CrashSolution(unsigned int num_var, unsigned int num_row) : primal(Vector(num_var)), rowact(Vector(num_row)) {}

   static CrashSolution read(std::string filename) {

   } 
};

#endif