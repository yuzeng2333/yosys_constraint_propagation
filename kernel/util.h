#ifndef UTIL
#define UTIL
using namespace z3;

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

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


std::string toStr(int i);

void print_cell(RTLIL::Cell* cell);

void print_sigspec(RTLIL::SigSpec connSig);
void print_IdString(RTLIL::IdString id);

void print_module(RTLIL::Module *module);
bool cell_is_module(Design* design, RTLIL::Cell* cell);
bool cell_is_bitAnd(Design* design, RTLIL::Cell* cell);
bool complete_signal(RTLIL::SigSpec sig);
bool equal_width(RTLIL::SigSpec sig1, RTLIL::SigSpec sig2);
RTLIL::SigSpec get_cell_port(RTLIL::SigSpec sig, RTLIL::Cell *cell);
RTLIL::Module* get_subModule(Design* design, RTLIL::Cell* cell);
RTLIL::SigSpec get_sigspec(RTLIL::Module* module, 
                           std::string inputName, int offset, int length);

std::string get_path(const std::vector<RTLIL::Cell*> &cell_stack = g_cell_stack);
std::string get_hier_name(RTLIL::SigSpec inputSig);
bool get_bit(uint32_t value, uint32_t pos);
void add_neq_ctrd(solver &s, context &c, RTLIL::SigSpec inputSig, int forbidValue);

void traverse(Design* design, RTLIL::Module* module);
expr get_expr(context &c, RTLIL::SigSpec sig, std::string path="");

PRIVATE_NAMESPACE_END

#endif
