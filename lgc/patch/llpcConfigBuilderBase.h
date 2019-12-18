/*
 ***********************************************************************************************************************
 *
 *  Copyright (c) 2019-2020 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 **********************************************************************************************************************/
/**
 ***********************************************************************************************************************
 * @file  llpcConfigBuilderBase.h
 * @brief LLPC header file: contains declaration of class lgc::ConfigBuilderBase.
 ***********************************************************************************************************************
 */
#pragma once

#include "llpcAbiMetadata.h"
#include "llpcTargetInfo.h"
#include "lgc/llpcBuilderCommon.h"
#include "llvm/BinaryFormat/MsgPackDocument.h"

namespace llvm
{

class LLVMContext;
class Module;

} // llvm

namespace lgc
{

class PipelineState;

// Invalid metadata key and value which shouldn't be exported to ELF.
constexpr uint32_t InvalidMetadataKey   = 0xFFFFFFFF;
constexpr uint32_t InvalidMetadataValue = 0xBAADBEEF;

struct PalMetadataNoteEntry
{
    uint32_t key;
    uint32_t value;
};

// =====================================================================================================================
// Register configuration builder base class.
class ConfigBuilderBase
{
public:
    ConfigBuilderBase(llvm::Module* pModule, PipelineState* pPipelineState);
    ~ConfigBuilderBase();

    void WritePalMetadata();

protected:
    void AddApiHwShaderMapping(ShaderStage apiStage, uint32_t hwStages);

    uint32_t SetShaderHash(ShaderStage apiStage);
    void SetNumAvailSgprs(Util::Abi::HardwareStage hwStage, uint32_t value);
    void SetNumAvailVgprs(Util::Abi::HardwareStage hwStage, uint32_t value);
    void SetUsesViewportArrayIndex(bool useViewportIndex);
    void SetPsUsesUavs(bool value);
    void SetPsWritesUavs(bool value);
    void SetPsWritesDepth(bool value);
    void SetEsGsLdsByteSize(uint32_t value);
    void SetCalcWaveBreakSizeAtDrawTime(bool value);
    void SetWaveFrontSize(Util::Abi::HardwareStage hwStage, uint32_t value);
    void SetApiName(const char* pValue);
    void SetPipelineType(Util::Abi::PipelineType value);
    void SetLdsSizeByteSize(Util::Abi::HardwareStage hwStage, uint32_t value);
    void SetEsGsLdsSize(uint32_t value);
    uint32_t SetupFloatingPointMode(ShaderStage shaderStage);

    void AppendConfig(llvm::ArrayRef<PalMetadataNoteEntry> config);
    void AppendConfig(uint32_t key, uint32_t value);

    template<typename T>
    void AppendConfig(const T& config)
    {
        static_assert(T::ContainsPalAbiMetadataOnly,
                      "may only be used with structs that are fully metadata notes");
        static_assert(sizeof(T) % sizeof(PalMetadataNoteEntry) == 0,
                      "T claims to be isPalAbiMetadataOnly, but sizeof contradicts that");

        AppendConfig({reinterpret_cast<const PalMetadataNoteEntry*>(&config),
                      sizeof(T) / sizeof(PalMetadataNoteEntry)});
    }

    // -----------------------------------------------------------------------------------------------------------------

    llvm::Module*                   m_pModule;            // LLVM module being processed
    llvm::LLVMContext*              m_pContext;           // LLVM context
    PipelineState*                  m_pPipelineState;     // Pipeline state
    GfxIpVersion                    m_gfxIp;              // Graphics IP version info

    bool                            m_hasVs;              // Whether the pipeline has vertex shader
    bool                            m_hasTcs;             // Whether the pipeline has tessellation control shader
    bool                            m_hasTes;             // Whether the pipeline has tessellation evaluation shader
    bool                            m_hasGs;              // Whether the pipeline has geometry shader

    uint32_t                        m_userDataLimit;      // User data limit for shaders seen so far
    uint32_t                        m_spillThreshold;     // Spill threshold for shaders seen so far

private:
    // Get the MsgPack map node for the specified API shader in the ".shaders" map
    llvm::msgpack::MapDocNode GetApiShaderNode(uint32_t apiStage);
    // Get the MsgPack map node for the specified HW shader in the ".hardware_stages" map
    llvm::msgpack::MapDocNode GetHwShaderNode(Util::Abi::HardwareStage hwStage);
    // Set USER_DATA_LIMIT (called once for the whole pipeline)
    void SetUserDataLimit();
    // Set SPILL_THRESHOLD (called once for the whole pipeline)
    void SetSpillThreshold();
    // Set PIPELINE_HASH (called once for the whole pipeline)
    void SetPipelineHash();

    // -----------------------------------------------------------------------------------------------------------------
    std::unique_ptr<llvm::msgpack::Document>  m_document;       // The MsgPack document
    llvm::msgpack::MapDocNode                 m_pipelineNode;   // MsgPack map node for amdpal.pipelines[0]
    llvm::msgpack::MapDocNode                 m_apiShaderNodes[ShaderStageNativeStageCount];
                                                                // MsgPack map node for each API shader's node in
                                                                //  ".shaders"
    llvm::msgpack::MapDocNode                 m_hwShaderNodes[uint32_t(Util::Abi::HardwareStage::Count)];
                                                                // MsgPack map node for each HW shader's node in
                                                                //  ".hardware_stages"

    llvm::SmallVector<PalMetadataNoteEntry, 128> m_config; // Register/metadata configuration
};

} // lgc