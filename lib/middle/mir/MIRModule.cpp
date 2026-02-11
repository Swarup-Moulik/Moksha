#include "moksha/MIR/MIRModule.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRGlobal.h"
#include <iostream>

namespace moksha {
namespace mir {

MIRModule::MIRModule(std::string name) : name(std::move(name)) {}

MIRModule::~MIRModule() = default;

void MIRModule::addFunction(std::unique_ptr<MIRFunction> func) {
  functions.push_back(std::move(func));
}

MIRFunction *MIRModule::getFunction(const std::string &name) const {
  for (const auto &f : functions) {
    if (f->getName() == name)
      return f.get();
  }
  return nullptr;
}

void MIRModule::addGlobal(std::unique_ptr<MIRGlobal> global) {
  globals.push_back(std::move(global));
}

MIRGlobal *MIRModule::getGlobal(const std::string &name) const {
  for (const auto &g : globals) {
    if (g->getName() == name)
      return g.get();
  }
  return nullptr;
}

void MIRModule::dump(std::ostream &os) const {
  os << "; ModuleID = '" << getName() << "'\n\n";

  for (const auto &g : getGlobals()) {
    g->dump(os);
    os << "\n";
  }
  if (!getGlobals().empty())
    os << "\n";

  for (const auto &f : getFunctions()) {
    f->dump(os);
    os << "\n";
  }
}

} // namespace mir
} // namespace moksha
