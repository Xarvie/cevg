// Stub for SkSLSPIRVValidator — spirv-tools not integrated
// Provides the symbols needed by VulkanSpirvTransforms.cpp

#include "src/sksl/codegen/SkSLSPIRVValidator.h"
#include "src/sksl/SkSLErrorReporter.h"
#include "include/private/base/SkSpan_impl.h"

namespace SkSL {

bool ValidateSPIRV(ErrorReporter&, SkSpan<const uint32_t>) {
    // SPIRV validation skipped (spirv-tools not available)
    return true;
}

bool ValidateSPIRVAndDissassemble(ErrorReporter&, SkSpan<const uint32_t>) {
    return true;
}

}  // namespace SkSL
