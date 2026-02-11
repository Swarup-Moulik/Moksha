#ifndef MOKSHA_DIALECT
#define MOKSHA_DIALECT

include "mlir/IR/OpBase.td"

def Moksha_Dialect : Dialect {
    let name = "moksha";
    let summary = "The Moksha Language Dialect.";
    let description = [{
        The Moksha dialect serves as the bridge between ownership-resolved MIR
        and final LLVM lowering. It provides managed types and ARC operations.
    }];
    let cppNamespace = "::moksha";

    // Enables TableGen to generate boilerplate for printing and parsing types.
    let useDefaultTypePrinterParser = 1;
}

// Base class for Moksha Types.
class Moksha_Type<string name, string typeMnemonic> : TypeDef<Moksha_Dialect, name> {
    let mnemonic = typeMnemonic;
}

// Base class for Moksha Operations.
class Moksha_Op<string mnemonic, list<Trait> traits = []> :
    Op<Moksha_Dialect, mnemonic, traits>;

#endif // MOKSHA_DIALECT
