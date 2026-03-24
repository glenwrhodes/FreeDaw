/*
 * Minimal VST 2.4 interface definitions for hosting VST2 plug-ins.
 *
 * These are binary-compatible type declarations that describe the ABI
 * already exported by every VST2 DLL.  They do NOT contain any
 * Steinberg implementation code.
 *
 * Based on the publicly documented VST 2.4 specification.
 */

#ifndef __aeffect__
#define __aeffect__

#include <cstdint>

#if defined(_WIN32)
  #define VSTCALLBACK __cdecl
#else
  #define VSTCALLBACK
#endif

#ifndef DECLARE_VST_DEPRECATED
  #define DECLARE_VST_DEPRECATED(x) x
#endif

typedef int32_t  VstInt32;
typedef intptr_t VstIntPtr;

struct AEffect;

typedef VstIntPtr (VSTCALLBACK *audioMasterCallback)
    (AEffect* effect, int32_t opcode, int32_t index, VstIntPtr value,
     void* ptr, float opt);

typedef VstIntPtr (VSTCALLBACK *AEffectDispatcherProc)
    (AEffect* effect, int32_t opcode, int32_t index, VstIntPtr value,
     void* ptr, float opt);

typedef void (VSTCALLBACK *AEffectProcessProc)
    (AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames);

typedef void (VSTCALLBACK *AEffectProcessDoubleProc)
    (AEffect* effect, double** inputs, double** outputs, int32_t sampleFrames);

typedef void (VSTCALLBACK *AEffectSetParameterProc)
    (AEffect* effect, int32_t index, float parameter);

typedef float (VSTCALLBACK *AEffectGetParameterProc)
    (AEffect* effect, int32_t index);

enum VstAEffectFlags
{
    effFlagsHasEditor      = 1 << 0,
    effFlagsCanReplacing   = 1 << 4,
    effFlagsProgramChunks  = 1 << 5,
    effFlagsIsSynth        = 1 << 8,
    effFlagsNoSoundInStop  = 1 << 9,
    effFlagsCanDoubleReplacing = 1 << 12
};

enum
{
    kEffectMagic = 0x56737450  // 'VstP'
};

#pragma pack(push, 8)
struct AEffect
{
    int32_t magic;
    AEffectDispatcherProc dispatcher;
    AEffectProcessProc    DECLARE_VST_DEPRECATED(process);
    AEffectSetParameterProc setParameter;
    AEffectGetParameterProc getParameter;

    int32_t numPrograms;
    int32_t numParams;
    int32_t numInputs;
    int32_t numOutputs;
    int32_t flags;

    VstIntPtr resvd1;
    VstIntPtr resvd2;

    int32_t initialDelay;

    int32_t DECLARE_VST_DEPRECATED(realQualities);
    int32_t DECLARE_VST_DEPRECATED(offQualities);
    float   DECLARE_VST_DEPRECATED(ioRatio);

    void*   object;
    void*   user;

    int32_t uniqueID;
    int32_t version;

    AEffectProcessProc processReplacing;
    AEffectProcessDoubleProc processDoubleReplacing;

    char future[56];
};
#pragma pack(pop)

#endif // __aeffect__
