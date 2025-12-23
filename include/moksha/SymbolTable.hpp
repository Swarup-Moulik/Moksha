#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>
#include "moksha/ast.hpp"

// Forward declare LLVM Value to avoid including large headers here
namespace llvm {
    class Value;
    class Type;
}

namespace Moksha {

// Represents the kind of symbol
enum class SymbolKind {
    VARIABLE,
    FUNCTION,
    CLASS,
    STRUCT,
    PARAMETER,
    ENUM,
    TYPEDEF
};

// Information stored about each symbol
struct SymbolInfo {
    std::string name;
    std::string type; // E.g., "int", "string", "MyClass"
    SymbolKind kind;
    bool isConst;
    bool isNullable;

    // --- Systems Metadata ---
    bool isVolatile = false;  
    bool isExtern = false;    
    int alignment = 0;        
    std::string section = "";
    
    // This holds the actual LLVM memory address (AllocaInst*) or Function* // generated during Phase 4 (CodeGen).
    llvm::Value* address = nullptr; 
    llvm::Type* llvmType = nullptr;

    // Store the high-level Moksha Type (preserves Array info, e.g., int[])
    std::shared_ptr<Type> resolvedType = nullptr;
    int scopeDepth = 0; 
    VariableDeclarationNode* declNode = nullptr;
    std::shared_ptr<Type> aliasTarget = nullptr;
};

class SymbolTable {
private:
    // A single scope is a map from name -> SymbolInfo
    using Scope = std::unordered_map<std::string, SymbolInfo>;
    std::vector<Scope> scopes;

public:
    SymbolTable() {
        // Always start with a global scope
        enterScope();
    }

    std::map<std::string, std::shared_ptr<Type>> typeAliases;

    void defineType(const std::string& name, std::shared_ptr<Type> type) {
        typeAliases[name] = type;
    }

    void defineTypedef(const std::string& name, std::shared_ptr<Type> targetType) {
        if (scopes.empty()) return;
        
        SymbolInfo info;
        info.name = name;
        info.kind = SymbolKind::TYPEDEF; // Use the new kind
        info.aliasTarget = targetType;   // Store the type
        info.scopeDepth = getCurrentDepth();
        
        // Add to the CURRENT scope
        scopes.back()[name] = info;
    }

    std::shared_ptr<Type> resolveType(const std::string& name) {
        // 1. Check Scoped Typedefs (if any)
        SymbolInfo* info = resolve(name);
        if (info && info->kind == SymbolKind::TYPEDEF) {
            return info->aliasTarget;
        }

        // 2. Check Global Type Aliases (The missing link!)
        if (typeAliases.count(name)) {
            return typeAliases[name];
        }

        return nullptr;
    }

    int getCurrentDepth() const {
        return static_cast<int>(scopes.size());
    }

    // Create a new scope (e.g., entering a function or block)
    void enterScope() {
        scopes.emplace_back();
    }

    // Destroy the current scope (e.g., leaving a block)
    void exitScope() {
        if (scopes.empty()) {
            throw std::runtime_error("Internal Compiler Error: SymbolTable underflow");
        }
        scopes.pop_back();
    }

    // Define a new symbol in the current scope
    bool define(const std::string& name, const std::string& type, SymbolKind kind, 
                llvm::Value* address = nullptr, llvm::Type* llvmType = nullptr,
                bool isConst = false, bool isNullable = false,
                std::shared_ptr<Type> resolvedType = nullptr, 
                VariableDeclarationNode* declNode = nullptr,
                bool isVol = false, bool isExt = false, 
                int align = 0, std::string sect = "") { 
        
        if (scopes.empty()) return false;

        auto& currentScope = scopes.back();
        if (currentScope.count(name)) return false;

        SymbolInfo info;
        info.name = name; info.type = type; info.kind = kind;
        info.isConst = isConst; info.isNullable = isNullable;
        info.address = address; info.llvmType = llvmType;
        info.resolvedType = resolvedType; info.scopeDepth = getCurrentDepth();
        info.declNode = declNode;
        info.isVolatile = isVol; info.isExtern = isExt; 
        info.alignment = align; info.section = sect;   

        currentScope[name] = info;
        return true;
    }

    // Look up a symbol, starting from the innermost scope and going outward
    SymbolInfo* resolve(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;
    }

    // Check if a symbol exists in the *current* scope only
    bool isDefinedInCurrentScope(const std::string& name) {
        if (scopes.empty()) return false;
        return scopes.back().count(name) > 0;
    }
};

} // namespace Moksha