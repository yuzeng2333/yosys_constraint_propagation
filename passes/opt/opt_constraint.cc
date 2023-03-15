/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Claire Xenia Wolf <claire@yosyshq.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *  ---
 *
 *
 */

#include "ctrd_prop.h"
//#include "util.h"

using namespace z3;

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN


std::string toStr(int i) {
  return std::to_string(i);
}


void print_cell(RTLIL::Cell* cell) {
  std::cout << "cell name: " << cell->name.str() << std::endl;
  std::cout << "cell type: " << cell->type.str() << std::endl;
}

void print_sigspec(RTLIL::SigSpec connSig) {
  if(connSig.is_wire()) {
    auto wire = connSig.as_wire();
    std::string wireName = wire->name.str();
    if(wireName == "\\io_opcode") std::cout << "find io_opcode" << std::endl;
    std::cout << "wire: " << wireName << std::endl;
  }
  else if(connSig.is_fully_const()) {
    std::cout << "const: " << connSig.as_int() << std::endl;    
  }
}

void print_IdString(RTLIL::IdString id) {
  std::cout << "IdString: " << id.str() << std::endl;
}

void print_module(RTLIL::Module *module) {
  std::cout << "module: " << module->name.str() << std::endl;
}


bool cell_is_module(Design* design, RTLIL::Cell* cell) {
  RTLIL::IdString type = cell->type;
  std::string typeName = type.str();
  if(typeName.front() != '$' && design->modules_.find(type) != design->modules_.end())
    return true;
  else return false;
}


bool cell_is_bitAnd(Design* design, RTLIL::Cell* cell) {
  RTLIL::IdString type = cell->type;
  std::string typeName = type.str();
  if(typeName.front() != '$' && design->modules_.find(type) != design->modules_.end())
    return true;
  else return false;
}

bool complete_signal(RTLIL::SigSpec sig) {
  return !sig.is_chunk() && !sig.is_bit();
}


bool equal_width(RTLIL::SigSpec sig1, RTLIL::SigSpec sig2) {
  return sig1.size() == sig2.size();
}


RTLIL::SigSpec get_cell_port(RTLIL::SigSpec sig, RTLIL::Cell *cell) {
  for(auto pair : cell->connections_) {
    auto portId = pair.first;
    auto connSig = pair.second;
    // only consider one case here:
    // 1. port and sig are perfectly connected
    assert(complete_signal(connSig));
    assert(complete_signal(sig));
    if(connSig == sig) return cell->getPort(portId);
  }
  return RTLIL::SigSpec();
}


RTLIL::Module* get_subModule(Design* design, RTLIL::Cell* cell) {
  return design->modules_[cell->type];
}


RTLIL::SigSpec get_sigspec(RTLIL::Module* module, 
                           std::string inputName, int offset, int length) {
  for(auto pair: module->wires_) {
    auto id = pair.first;
    auto wire = pair.second;
    if(wire->name.str() == inputName) {
      std::cout << "wire width: " << wire->width << std::endl;
      std::cout << "offset: " << offset << std::endl;
      std::cout << "length: " << length << std::endl;
      return RTLIL::SigSpec(wire, offset, length);
    }
  }
}


std::string get_path() {
  std::string path;
  bool first = true;
  for(auto cell: g_cell_stack) {
    if(first) {
      path = cell->name.str();
      first = false;
    }
    else {
      path += "." + cell->name.str();
    }
  }
  return path;
}


std::string get_hier_name(RTLIL::SigSpec inputSig) {
  //assert(!inputSig.is_bit());
  std::string path = get_path();
  std::string wireName;
  wireName = inputSig.as_wire()->name.str();
  return path + "." + wireName;
}


bool get_bit(uint32_t value, uint32_t pos) {
  assert(pos < 32);
  int bitVal = (value & (1 << pos));
  return bitVal > 0;
}


void add_neq_ctrd(solver &s, context &c, RTLIL::SigSpec inputSig, int forbidValue) {
  //assert(complete_signal(inputSig));
  std::string inputName = get_hier_name(inputSig);
  int width = inputSig.size();
  //if(g_expr_map.find(inputName) != g_expr_map.end()) {
  //  expr inputExpr = g_expr_map[inputName];
  //  s.add(inputExpr != forbidValue);
  //}
  //else {
    expr inputExpr = c.bv_const(inputName.c_str(), width);
    s.add(inputExpr != forbidValue);
  //}
  std::cout << "-- Add input constraint, the model is: " << s.to_smt2() << std::endl;
}


//void add_neq_bits_ctrd(solver &s, context &c, RTLIL::SigSpec inputSig, uint32_t forbidValue) {
//  inputSig.unpack();
//  int pos = 0;
//  std::string inputName = get_hier_name(inputSig);
//  bool first = true;
//  expr disjunct;
//  for(auto bit: inputSig.bits_) {
//    std::string bitName = inputName + ".bit" + toStr(pos);
//    expr x = c.bool_const(bitName);
//    bool val = get_bit(forbidValue, pos++);
//    if(first) {
//      first = false;
//      disjunct = (x != val);
//    }
//    else {
//      disjunct = disjunct || (x != val);
//    }
//  }
//  s.add(disjunct);
//}


expr get_expr(context &c, RTLIL::SigSpec sig, std::string path) {
  int width = sig.size();
  std::string name;
  if(path.empty()) name = get_hier_name(sig);  
  else name = path + "." + sig.as_wire()->name.str();
  if(sig.is_wire()) {
    if(g_expr_map.find(name) != g_expr_map.end())
      return *g_expr_map[name];
    else {
      expr ret = width > 1 ? c.bv_const(name.c_str(), width) : c.bool_const(name.c_str());
      g_expr_map.emplace(name, std::make_shared<expr>(ret));
      return ret;
    }
  }
  else if(sig.is_chunk()) {
    auto chunk = sig.as_chunk();
    int offset = chunk.offset;
    if(g_expr_map.find(name) != g_expr_map.end()) {
      auto completeExpr = g_expr_map[name];
      return completeExpr->extract(width+offset-1, offset);
    }
    else {
      int fullWidth = sig.size();
      expr ret = width > 1 ? c.bv_const(name.c_str(), fullWidth) : c.bool_const(name.c_str());
      g_expr_map.emplace(name, std::make_shared<expr>(ret));
      return ret.extract(width+offset-1, offset);
    }
  }
}


void traverse(Design* design, RTLIL::Module* module) {
  std::cout << "=== Begin a new module:"  << std::endl;
  print_module(module);
  // traverse all cells
  for(auto cellPair : module->cells_) {
    RTLIL::IdString IdStr = cellPair.first;
    RTLIL::Cell* cell = cellPair.second;
    print_module(cell->module);
    print_cell(cell);
    bool use_ctrd_sig = false;
    bool use_forbid_value = false;
    RTLIL::SigSpec outputWire;
    RTLIL::IdString outputPort;
    for(auto &conn: cell->connections_) {
      RTLIL::IdString port = conn.first;
      RTLIL::SigSpec connSig = conn.second;
      print_sigspec(connSig);
    }
  }
}


void collect_eq(RTLIL::Cell* cell, RTLIL::SigSpec ctrdSig) {
  std::cout << "-- collect_eq: " << ctrdSig.as_wire()->name.str() << std::endl;
  bool use_ctrd_sig = false;
  bool use_const = false;
  int constValue;
  RTLIL::SigSpec outputWire;
  for(auto &conn: cell->connections_) {
    RTLIL::IdString port = conn.first;
    RTLIL::SigSpec connSig = conn.second;
    if(cell->input(port)) {
      if(connSig.is_wire() && connSig == ctrdSig) {
        use_ctrd_sig = true;
      }
      else if(connSig.is_fully_const()) {
        use_const = true;
        constValue = connSig.as_int();
      }
    }
    else {
      assert(cell->output(port));
      outputWire = connSig;
    }
  }
  if(use_ctrd_sig && use_const) {
    std::string path = get_path();
    g_check_vec.push_back(CheckSet{path, cell, outputWire, ctrdSig, constValue});
  }
}


void simplify_eq(solver &s, context &c, RTLIL::Module* module, 
                 RTLIL::Cell* cell, RTLIL::SigSpec ctrdSig, int forbidValue) {
  print_cell(cell);
  bool use_ctrd_sig = false;
  bool use_forbid_value = false;
  RTLIL::SigSpec outputWire;
  RTLIL::IdString outputPort;
  for(auto &conn: cell->connections_) {
    RTLIL::IdString port = conn.first;
    RTLIL::SigSpec connSig = conn.second;
    print_sigspec(connSig);
    if(cell->input(port)) {
      if(connSig.is_wire() && connSig == ctrdSig) {
        use_ctrd_sig = true;
      }
      else if(connSig.is_fully_const()) {
        int eqValue = connSig.as_int();
        use_forbid_value = eqValue == forbidValue;
      }
    }
    else {
      assert(cell->output(port));
      outputWire = connSig;
      outputPort = port;
    }
  }
  // replace the eq with constant false
  if(use_ctrd_sig && use_forbid_value) {
    module->remove(cell);
    //cell->connections_[outputPort] = RTLIL::SigSpec();
    // connect the output wire to always false
    module->connect(outputWire, RTLIL::SigSpec(false));
  }
}


void get_drive_map(RTLIL::Module* module, DriveMap_t &mp) {
  for(auto cellPair: module->cells_) {
    RTLIL::Cell* cell = cellPair.second;
    for(auto pair: cell->connections_) {
      auto portId = pair.first;
      auto connSig = pair.second;
      if(mp.find(connSig) == mp.end())
        mp.emplace(connSig, DestGroup{std::set<RTLIL::SigSpec>{}, std::set<RTLIL::Cell*>{cell}});
      else
        mp[connSig].cells.insert(cell);
    }
  }
}


void add_submod(solver &s, context &c, RTLIL::Design* design, RTLIL::Module* module, 
                RTLIL::Cell* cell, RTLIL::SigSpec ctrdSig) {
   std::cout << "-- add_submod, cell: " << cell->name.str() << ", ctrdSig: " 
             << ctrdSig.as_wire()->name.str() << std::endl;   
   RTLIL::SigSpec port = get_cell_port(ctrdSig, cell);
   if(port.empty()) return;
   auto subMod = get_subModule(design, cell);
   DriveMap_t mp;
   get_drive_map(subMod, mp);
   propagate_constraints(s, c, design, subMod, mp, port);
}


void add_and(solver &s, context &c, RTLIL::Design* design, RTLIL::Module* module, 
             DriveMap_t &mp, RTLIL::Cell* cell, RTLIL::SigSpec ctrdSig) {
  std::cout << "-- add_and, cell: " << cell->name.str() << ", ctrdSig: " 
            << ctrdSig.as_wire()->name.str() << std::endl;
  RTLIL::SigSpec port = get_cell_port(ctrdSig, cell);
  if(port.empty()) return;
  RTLIL::SigSpec outputConnSig;
  bool const_arg = false;
  int const_value;
  for(auto pair: cell->connections_) {
    auto portId = pair.first;
    auto connSig = pair.second;
    if(cell->input(portId) && connSig.is_fully_const()) {
      const_arg = true;
      const_value = connSig.as_int();
    }
    else if(cell->output(portId)) {
      outputConnSig = connSig;
    }
  }
  if(const_arg) {
    assert(equal_width(ctrdSig, outputConnSig));
    auto path = get_path();
    expr ctrdExpr = get_expr(c, ctrdSig, path);
    expr outExpr = get_expr(c, outputConnSig, path);
    s.add((ctrdExpr & const_value) == outExpr);
    propagate_constraints(s, c, design, module, mp, outputConnSig);
  }
}


/// Recursively propagate constraints through the design
void propagate_constraints(solver &s, context &c, Design* design, RTLIL::Module* module, 
                           DriveMap_t &mp, RTLIL::SigSpec ctrdSig)
                           //std::string ctrdSig, int offset, int length, uint32_t forbidValue)
{
  // traverse all connections
  //for(auto &conn : module->connections()) {
  //  auto srcSig = conn.first;
  //  //RTLIL::IdString srcName = srcSig.as_wire()->name;
  //  //if(ctdSig != srcSig) continue;
  //  auto dstSig = conn.second;
  //  if(dstSig.is_wire()) {
  //    g_work_list.push(std::make_pair(dstSig, forbidValue));
  //  }
  //}
  // traverse all cells
  print_sigspec(ctrdSig);
  assert(mp.find(ctrdSig) != mp.end());
  const auto group = mp[ctrdSig];
  assert(group.wires.empty());
  auto connectedCells = group.cells;
  for(auto cell: connectedCells) {
    if(cell->type == ID($eq)) 
      collect_eq(cell, ctrdSig);
    else if(cell_is_module(design, cell))
      add_submod(s, c, design, module, cell, ctrdSig);
    else if(cell->type == ID($and))
      add_and(s, c, design, module, mp, cell, ctrdSig);
    else {
      std::cout << "Error: unexpected cell type: " << cell->type.str() << std::endl;
    }
  }
}


void simplify_eq_constraint(solver &s, context &c) {
  std::cout << "=== Begin simplify" << std::endl;
  for(auto set: g_check_vec) {
    std::cout << "-- A new check" << std::endl;    
    std::string path = set.path;
    auto cell = set.cell;
    RTLIL::SigSpec outSig = set.outSig;
    RTLIL::SigSpec ctrdSig = set.ctrdSig;
    auto module = cell->module;
    int forbidValue = set.forbidValue;
    expr outExpr = get_expr(c, outSig, path);
    expr ctrdExpr = get_expr(c, ctrdSig, path);
    s.push();
    s.add(ctrdExpr == forbidValue);
    std::cout << "-- To check SAT, the model is: " << s.to_smt2() << std::endl;    
    if(s.check() == unsat) {
      std::cout << "!!! UNSAT! do simplification" << std::endl;
      module->remove(cell);
      module->connect(outSig, RTLIL::SigSpec(false));
    }
    s.pop();
  }
}


struct ConstraintPropagatePass : public Pass {
  ConstraintPropagatePass() : Pass("opt_ctrd", "constraint propagation pass") { }
  void execute(std::vector<std::string> args, Design* design) override { 
    log_header(design, "Executing the new OPT_CONSTRAINT pass\n");
    std::cout << "Inside ConstraintPropagatePass" << std::endl;
    // parse args
    std::string inputName = "\\io_opcode";
    int shift = 0;
    int length = 2;
    uint32_t forbidValue = 1;    
    for (int argidx = 1; argidx < args.size(); argidx++) {
			if (args[argidx] == "-name") {
        assert(argidx < args.size() - 1);
				inputName = args[++argidx];
				continue;
			}
			if (args[argidx] == "-shift") {
        assert(argidx < args.size() - 1);
				shift = std::stoi(args[++argidx]);
				continue;
			}
			if (args[argidx] == "-length") {
        assert(argidx < args.size() - 1);
				length = std::stoi(args[++argidx]);
				continue;
			}
			if (args[argidx] == "-forbid") {
        assert(argidx < args.size() - 1);
				forbidValue = std::stoi(args[++argidx]);
				continue;
			}
    }

    // begin simplification
    context c;
    solver s(c);
    // Iterate through all modules in the design
    RTLIL::Module* module = design->top_module();
    // Recursively propagate constants through the module
    RTLIL::SigSpec inputSig = get_sigspec(module, inputName, shift, length);
    add_neq_ctrd(s, c, inputSig, forbidValue);
    DriveMap_t mp;
    get_drive_map(module, mp);
    std::cout << "=== Begin propagate_constraints" << std::endl;    
    propagate_constraints(s, c, design, module, mp, inputSig);
    simplify_eq_constraint(s, c);
  }
} ConstraintPropagatePass;


PRIVATE_NAMESPACE_END
