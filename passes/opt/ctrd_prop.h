#ifndef CTRD_PROP
#define CTRD_PROP

#include "kernel/register.h"
#include "kernel/celltypes.h"
#include "kernel/log.h"
#include "kernel/sigtools.h"
#include "kernel/ff.h"
#include "kernel/mem.h"
#include "kernel/rtlil.h"
#include <string>
#include <sstream>
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <assert.h>
#include <z3++.h>

using namespace z3;

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN


struct WorkItem {
  std::string sigName;
  int offset;
  int length;
  uint32_t forbidValue;
};


struct CheckSet {
  std::string path;
  RTLIL::Cell* cell;
  RTLIL::SigSpec outSig;
  RTLIL::SigSpec ctrdSig;
  int forbidValue;
};


struct DestGroup {
  std::set<RTLIL::SigSpec> wires;
  std::set<RTLIL::Cell*> cells;
};


std::queue<WorkItem> g_work_list;
std::vector<RTLIL::Cell*> g_cell_stack;
std::vector<CheckSet> g_check_vec;
std::map<std::string, expr*> g_expr_map;
typedef std::map<RTLIL::SigSpec, DestGroup> DriveMap_t;


void collect_eq(RTLIL::Cell* cell, RTLIL::SigSpec ctrdSig, int forbidValue);
void simplify_eq(solver &s, context &c, RTLIL::Module* module, 
                 RTLIL::Cell* cell, RTLIL::SigSpec ctrdSig, int forbidValue);
void add_submod(solver &s, context &c, RTLIL::Design* design, RTLIL::Module* module, 
                     RTLIL::Cell* cell, RTLIL::SigSpec ctrdSig);
void add_and(solver &s, context &c, RTLIL::Design* design, RTLIL::Module* module, 
             DriveMap_t &mp, RTLIL::Cell* cell, RTLIL::SigSpec ctrdSig);
void propagate_constraints(solver &s, context &c, Design* design, RTLIL::Module* module, 
                           DriveMap_t &mp, RTLIL::SigSpec ctrdSig);

void get_drive_map(RTLIL::Module* module, DriveMap_t &mp);

PRIVATE_NAMESPACE_END

#endif
