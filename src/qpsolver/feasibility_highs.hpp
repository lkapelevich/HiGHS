#ifndef __SRC_LIB_FEASIBILITYHIGHS_HPP__
#define __SRC_LIB_FEASIBILITYHIGHS_HPP__

#include "Highs.h"
#include "feasibility.hpp"

void computestartingpoint(Runtime& runtime, CrashSolution& result) {
  // compute initial feasible point
  Highs highs;

  // set HiGHS to be silent
  highs.setOptionValue("output_flag", false);
  highs.setOptionValue("Time limit", runtime.settings.timelimit -
                                         runtime.timer.readRunHighsClock());

  HighsLp lp;
  lp.a_matrix_.index_ =
      *((std::vector<HighsInt>*)&runtime.instance.A.mat.index);
  lp.a_matrix_.start_ =
      *((std::vector<HighsInt>*)&runtime.instance.A.mat.start);
  lp.a_matrix_.value_ = runtime.instance.A.mat.value;
  lp.a_matrix_.format_ = MatrixFormat::kColwise;
  lp.col_cost_.assign(runtime.instance.num_var, 0.0);
  // lp.col_cost_ = runtime.instance.c.value;
  lp.col_lower_ = runtime.instance.var_lo;
  lp.col_upper_ = runtime.instance.var_up;
  lp.row_lower_ = runtime.instance.con_lo;
  lp.row_upper_ = runtime.instance.con_up;
  lp.num_col_ = runtime.instance.num_var;
  lp.num_row_ = runtime.instance.num_con;
  highs.passModel(lp);

  HighsBasis basis;
  basis.alien = true;  // Set true when basis is instantiated
  HighsInt num_basic_col = 0;
  HighsInt num_nonbasic_col = 0;
  HighsInt num_basic_row = 0;
  HighsInt num_nonbasic_row = 0;
  for (HighsInt i = 0; i < runtime.instance.num_con; i++) {
    basis.row_status.push_back(HighsBasisStatus::kNonbasic);
    num_nonbasic_row++;
  }

  for (HighsInt i = 0; i < runtime.instance.num_var; i++) {
    // make free variables basic
    if (runtime.instance.var_lo[i] ==
            -std::numeric_limits<double>::infinity() &&
        runtime.instance.var_up[i] == std::numeric_limits<double>::infinity()) {
      // free variable
      basis.col_status.push_back(HighsBasisStatus::kBasic);
      num_basic_col++;
    } else {
      basis.col_status.push_back(HighsBasisStatus::kNonbasic);
      num_nonbasic_col++;
    }
  }

  const HighsBasis& internal_basis = highs.getBasis();
  const HighsInfo& info = highs.getInfo();
  HighsInt simplex_iteration_count0 = std::max(0, info.simplex_iteration_count);

  num_basic_col = 0;
  num_nonbasic_col = 0;
  num_basic_row = 0;
  num_nonbasic_row = 0;
  for (HighsInt i = 0; i < runtime.instance.num_con; i++) {
    if (internal_basis.row_status[i] == HighsBasisStatus::kBasic) {
      num_basic_row++;
    } else {
      num_nonbasic_row++;
    }
  }

  for (HighsInt i = 0; i < runtime.instance.num_var; i++) {
    if (internal_basis.col_status[i] == HighsBasisStatus::kBasic) {
      num_basic_col++;
    } else {
      num_nonbasic_col++;
    }
  }

  highs.setOptionValue("simplex_strategy", kSimplexStrategyPrimal);
  HighsStatus status = highs.run();
  if (status != HighsStatus::kOk) {
    runtime.status = ProblemStatus::ERROR;
    return;
  }
  num_basic_col = 0;
  num_nonbasic_col = 0;
  num_basic_row = 0;
  num_nonbasic_row = 0;
  for (HighsInt i = 0; i < runtime.instance.num_con; i++) {
    if (internal_basis.row_status[i] == HighsBasisStatus::kBasic) {
      num_basic_row++;
    } else {
      num_nonbasic_row++;
    }
  }

  for (HighsInt i = 0; i < runtime.instance.num_var; i++) {
    if (internal_basis.col_status[i] == HighsBasisStatus::kBasic) {
      num_basic_col++;
    } else {
      num_nonbasic_col++;
    }
  }

  runtime.statistics.phase1_iterations = highs.getSimplexIterationCount();

  HighsModelStatus phase1stat = highs.getModelStatus();
  if (phase1stat == HighsModelStatus::kInfeasible) {
    runtime.status = ProblemStatus::INFEASIBLE;
    return;
  }

  HighsSolution sol = highs.getSolution();
  HighsBasis bas = highs.getBasis();

  Vector x0(runtime.instance.num_var);
  Vector ra(runtime.instance.num_con);
  for (HighsInt i = 0; i < x0.dim; i++) {
    if (fabs(sol.col_value[i]) > 10E-5) {
      x0.value[i] = sol.col_value[i];
      x0.index[x0.num_nz++] = i;
    }
  }

  for (HighsInt i = 0; i < ra.dim; i++) {
    if (fabs(sol.row_value[i]) > 10E-5) {
      ra.value[i] = sol.row_value[i];
      ra.index[ra.num_nz++] = i;
    }
  }

  std::vector<HighsInt> initialactive;
  std::vector<HighsInt> initialinactive;
  std::vector<BasisStatus> atlower;
  for (HighsInt i = 0; i < bas.row_status.size(); i++) {
    if (bas.row_status[i] == HighsBasisStatus::kLower) {
      initialactive.push_back(i);
      atlower.push_back(BasisStatus::ActiveAtLower);
    } else if (bas.row_status[i] == HighsBasisStatus::kUpper) {
      initialactive.push_back(i);
      atlower.push_back(BasisStatus::ActiveAtUpper);
    } else if (bas.row_status[i] != HighsBasisStatus::kBasic) {
      // printf("row %d nonbasic\n", i);
      initialinactive.push_back(runtime.instance.num_con + i);
    } else {
      assert(bas.row_status[i] == HighsBasisStatus::kBasic);
    }
  }

  for (HighsInt i = 0; i < bas.col_status.size(); i++) {
    if (bas.col_status[i] == HighsBasisStatus::kLower) {
      initialactive.push_back(i + runtime.instance.num_con);
      atlower.push_back(BasisStatus::ActiveAtLower);
    } else if (bas.col_status[i] == HighsBasisStatus::kUpper) {
      initialactive.push_back(i + runtime.instance.num_con);
      atlower.push_back(BasisStatus::ActiveAtUpper);
    } else if (bas.col_status[i] == HighsBasisStatus::kZero) {
      // printf("col %" HIGHSINT_FORMAT " free and set to 0 %" HIGHSINT_FORMAT
      // "\n", i, (HighsInt)bas.col_status[i]);
      initialinactive.push_back(runtime.instance.num_con + i);
    } else if (bas.col_status[i] != HighsBasisStatus::kBasic) {
      // printf("Column %" HIGHSINT_FORMAT " basis stus %" HIGHSINT_FORMAT "\n",
      // i, (HighsInt)bas.col_status[i]);
    } else {
      assert(bas.col_status[i] == HighsBasisStatus::kBasic);
    }
  }

  assert(initialactive.size() + initialinactive.size() ==
         runtime.instance.num_var);

  for (HighsInt ia : initialinactive) {
    if (ia < runtime.instance.num_con) {
      printf("free row %d\n", ia);
      assert(runtime.instance.con_lo[ia] ==
             -std::numeric_limits<double>::infinity());
      assert(runtime.instance.con_up[ia] ==
             std::numeric_limits<double>::infinity());
    } else {
      // printf("free col %d\n", ia);
      assert(runtime.instance.var_lo[ia - runtime.instance.num_con] ==
             -std::numeric_limits<double>::infinity());
      assert(runtime.instance.var_up[ia - runtime.instance.num_con] ==
             std::numeric_limits<double>::infinity());
    }
  }

  result.rowstatus = atlower;
  result.active = initialactive;
  result.inactive = initialinactive;
  result.primal = x0;
  result.rowact = ra;
}

#endif
