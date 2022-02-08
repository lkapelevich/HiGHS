/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                       */
/*    This file is part of the HiGHS linear optimization suite           */
/*                                                                       */
/*    Written and engineered 2008-2022 at the University of Edinburgh    */
/*                                                                       */
/*    Available as open-source under the MIT License                     */
/*                                                                       */
/*    Authors: Julian Hall, Ivet Galabova, Leona Gottwald and Michael    */
/*    Feldmeier                                                          */
/*                                                                       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef HIGHS_NODE_QUEUE_H_
#define HIGHS_NODE_QUEUE_H_

#include <cassert>
#include <queue>
#include <set>
#include <vector>

#include "lp_data/HConst.h"
#include "mip/HighsDomainChange.h"
#include "util/HighsCDouble.h"
#include "util/HighsRbTree.h"

class HighsDomain;
class HighsLpRelaxation;

class HighsNodeQueue {
 public:
  struct OpenNode {
    std::vector<HighsDomainChange> domchgstack;
    std::vector<HighsInt> branchings;
    std::vector<std::set<std::pair<double, HighsInt>>::iterator> domchglinks;
    double lower_bound;
    double estimate;
    HighsInt depth;
    highs::RbTreeLinks<int64_t> lowerLinks;
    highs::RbTreeLinks<int64_t> hybridEstimLinks;

    OpenNode()
        : domchgstack(),
          branchings(),
          domchglinks(),
          lower_bound(-kHighsInf),
          estimate(-kHighsInf),
          depth(0),
          lowerLinks(),
          hybridEstimLinks() {}

    OpenNode(std::vector<HighsDomainChange>&& domchgstack,
             std::vector<HighsInt>&& branchings, double lower_bound,
             double estimate, HighsInt depth)
        : domchgstack(domchgstack),
          branchings(branchings),
          lower_bound(lower_bound),
          estimate(estimate),
          depth(depth),
          lowerLinks(),
          hybridEstimLinks() {}

    OpenNode& operator=(OpenNode&& other) = default;
    OpenNode(OpenNode&&) = default;

    OpenNode& operator=(const OpenNode& other) = delete;
    OpenNode(const OpenNode&) = delete;
  };

  void checkGlobalBounds(HighsInt col, double lb, double ub, double feastol,
                         HighsCDouble& treeweight);

 private:
  class NodeLowerRbTree;
  class NodeHybridEstimRbTree;
  class SuboptimalNodeRbTree;

  std::vector<OpenNode> nodes;
  std::vector<std::set<std::pair<double, HighsInt>>> colLowerNodes;
  std::vector<std::set<std::pair<double, HighsInt>>> colUpperNodes;
  std::priority_queue<HighsInt, std::vector<HighsInt>, std::greater<HighsInt>>
      freeslots;
  int64_t lowerRoot = -1;
  int64_t lowerMin = -1;
  int64_t hybridEstimRoot = -1;
  int64_t hybridEstimMin = -1;
  int64_t suboptimalRoot = -1;
  int64_t suboptimalMin = -1;
  int64_t numSuboptimal = 0;
  double optimality_limit = kHighsInf;

  void link_estim(int64_t node);

  void unlink_estim(int64_t node);

  void link_lower(int64_t node);

  void unlink_lower(int64_t node);

  void link_suboptimal(int64_t node);

  void unlink_suboptimal(int64_t node);

  void link_domchgs(int64_t node);

  void unlink_domchgs(int64_t node);

  double link(int64_t node);

  void unlink(int64_t node);

 public:
  void setOptimalityLimit(double optimality_limit) {
    this->optimality_limit = optimality_limit;
  }

  double performBounding(double upper_limit);

  void setNumCol(HighsInt numcol);

  double emplaceNode(std::vector<HighsDomainChange>&& domchgs,
                     std::vector<HighsInt>&& branchings, double lower_bound,
                     double estimate, HighsInt depth);

  OpenNode&& popBestNode();

  OpenNode&& popBestBoundNode();

  int64_t numNodesUp(HighsInt col) const { return colLowerNodes[col].size(); }

  int64_t numNodesDown(HighsInt col) const { return colUpperNodes[col].size(); }

  int64_t numNodesUp(HighsInt col, double val) const {
    assert((HighsInt)colLowerNodes.size() > col);
    auto it = colLowerNodes[col].upper_bound(std::make_pair(val, kHighsIInf));
    if (it == colLowerNodes[col].begin()) return colLowerNodes[col].size();
    return std::distance(it, colLowerNodes[col].end());
  }

  int64_t numNodesDown(HighsInt col, double val) const {
    assert((HighsInt)colUpperNodes.size() > col);
    auto it = colUpperNodes[col].lower_bound(std::make_pair(val, -1));
    if (it == colUpperNodes[col].end()) return colUpperNodes[col].size();
    return std::distance(colUpperNodes[col].begin(), it);
  }

  const std::set<std::pair<double, HighsInt>>& getUpNodes(HighsInt col) const {
    return colLowerNodes[col];
  }

  const std::set<std::pair<double, HighsInt>>& getDownNodes(
      HighsInt col) const {
    return colUpperNodes[col];
  }

  double pruneInfeasibleNodes(HighsDomain& globaldomain, double feastol);

  double pruneNode(int64_t nodeId);

  double getBestLowerBound() const;

  HighsInt getBestBoundDomchgStackSize() const;

  void clear() {
    HighsNodeQueue nodequeue;
    nodequeue.setNumCol(colUpperNodes.size());
    *this = std::move(nodequeue);
  }

  int64_t numNodes() const { return nodes.size() - freeslots.size(); }

  int64_t numActiveNodes() const {
    return nodes.size() - freeslots.size() - numSuboptimal;
  }

  bool empty() const { return numActiveNodes() == 0; }
};

#endif
