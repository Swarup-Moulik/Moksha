// include/moksha/Dialect/MokshaDialect.td
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
    let cppNamespace = "moksha::IR";

    // Generate boilerplate for types and attributes
    let useDefaultTypePrinterParser = 1;

    // Enable custom materialization of constants (needed for `DecimalLiteral`)
    let hasConstantMaterializer = 1;
    let usePropertiesForAttributes = 0;
}

// Base class for Moksha Types.
class Moksha_Type<string name, string typeMnemonic> : TypeDef<Moksha_Dialect, name> {
    let mnemonic = typeMnemonic;
    let genStorageClass = 1;
}

// Base class for Moksha Operations.
class Moksha_Op<string mnemonic, list<Trait> traits = []> :
    Op<Moksha_Dialect, mnemonic, traits>;

#endif // MOKSHA_DIALECT
