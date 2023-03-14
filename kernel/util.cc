#include "ctrd_prop.h"

using namespace z3;

YOSYS_NAMESPACE_BEGIN

/// utils
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
      return RTLIL::SigSpec(wire, offset, length);
    }
  }
}


std::string get_path(const std::vector<RTLIL::Cell*> &cell_stack = g_cell_stack) {
  std::string path;
  bool first = true;
  for(auto cell: cell_stack) {
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
  assert(!inputSig.is_bit());
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
  assert(complete_signal(inputSig));
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


void traverse(Design* design, RTLIL::Module* module)
{
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


expr get_expr(context &c, RTLIL::SigSpec sig, std::string path) {
  int width = sig.size();
  std::string name;
  if(path.empty()) name = get_hier_name(sig);  
  else name = sig.as_wire()->name.str();
  if(sig.is_wire()) {
    if(g_expr_map.find(name) != g_expr_map.end())
      return *g_expr_map[name];
    else {
      expr ret = width > 1 ? c.bv_const(name.c_str(), width) : c.bool_const(name.c_str());
      g_expr_map.emplace(name, &ret);
      return ret;
    }
  }
  else if(sig.is_chunk()) {
    auto chunk = sig.as_chunk();
    int offset = chunk.offset;
    if(g_expr_map.find(name) != g_expr_map.end()) {
      expr* completeExpr = g_expr_map[name];
      return completeExpr->extract(width+offset-1, offset);
    }
    else {
      int fullWidth = sig.size();
      expr ret = width > 1 ? c.bv_const(name.c_str(), fullWidth) : c.bool_const(name.c_str());
      g_expr_map.emplace(name, &ret);
      return ret.extract(width+offset-1, offset);
    }
  }
}

YOSYS_NAMESPACE_END

