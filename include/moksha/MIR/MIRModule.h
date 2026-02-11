#pragma once

#include "moksha/MIR/MIRFunction.h"
#include <iosfwd> // Added
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace moksha {
namespace mir {

class MIRGlobal; // Forward decl

class MIRModule {
public:
  // [FIX] Removed MIRContext& param
  explicit MIRModule(std::string name);
  ~MIRModule();

  const std::string &getName() const { return name; }

  // Function Management
  void addFunction(std::unique_ptr<MIRFunction> func);
  MIRFunction *getFunction(const std::string &name) const;
  const std::vector<std::unique_ptr<MIRFunction>> &getFunctions() const {
    return functions;
  }

  // Global Variable Management
  void addGlobal(std::unique_ptr<MIRGlobal> global);
  MIRGlobal *getGlobal(const std::string &name) const;
  const std::vector<std::unique_ptr<MIRGlobal>> &getGlobals() const {
    return globals;
  }

  // Legacy Helpers
  MIRGlobal *findGlobalByName(const std::string &name) const {
    return getGlobal(name);
  }
  MIRFunction *findFunctionByName(const std::string &name) const {
    return getFunction(name);
  }

  void dump(std::ostream &os) const;

private:
  std::string name;

  std::vector<std::unique_ptr<MIRFunction>> functions;
  std::vector<std::unique_ptr<MIRGlobal>> globals;

  std::unordered_map<std::string, MIRFunction *> functionMap;
  std::unordered_map<std::string, MIRGlobal *> globalMap;
};

} // namespace mir
} // namespace moksha
