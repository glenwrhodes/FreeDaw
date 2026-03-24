/*
 * Minimal VST 2.4 extended interface definitions for hosting VST2 plug-ins.
 *
 * Binary-compatible type declarations describing the ABI exported by VST2 DLLs.
 * Based on the publicly documented VST 2.4 specification.
 */

#ifndef __aeffectx__
#define __aeffectx__

#include "aeffect.h"

enum VstStringConstants
{
    kVstMaxProgNameLen  = 24,
    kVstMaxParamStrLen  = 8,
    kVstMaxVendorStrLen = 64,
    kVstMaxProductStrLen = 64,
    kVstMaxEffectNameLen = 32,
    kVstMaxNameLen      = 64,
    kVstMaxLabelLen     = 64,
    kVstMaxShortLabelLen = 8,
    kVstMaxCategLabelLen = 24,
    kVstMaxFileNameLen  = 100
};

// ---- Dispatcher opcodes (host -> plugin) ----
enum
{
    effOpen = 0,
    effClose,
    effSetProgram,
    effGetProgram,
    effSetProgramName,
    effGetProgramName,
    effGetParamLabel,
    effGetParamDisplay,
    effGetParamName,
    DECLARE_VST_DEPRECATED(effGetVu),
    effSetSampleRate,
    effSetBlockSize,
    effMainsChanged,
    effEditGetRect,
    effEditOpen,
    effEditClose,
    DECLARE_VST_DEPRECATED(effEditDraw),
    DECLARE_VST_DEPRECATED(effEditMouse),
    DECLARE_VST_DEPRECATED(effEditKey),
    effEditIdle,
    DECLARE_VST_DEPRECATED(effEditTop),
    DECLARE_VST_DEPRECATED(effEditSleep),
    DECLARE_VST_DEPRECATED(effIdentify),
    effGetChunk,
    effSetChunk,
    effProcessEvents,
    effCanBeAutomated,
    effString2Parameter,
    DECLARE_VST_DEPRECATED(effGetNumProgramCategories),
    effGetProgramNameIndexed,
    DECLARE_VST_DEPRECATED(effCopyProgram),
    DECLARE_VST_DEPRECATED(effConnectInput),
    DECLARE_VST_DEPRECATED(effConnectOutput),
    effGetInputProperties,
    effGetOutputProperties,
    effGetPlugCategory,
    DECLARE_VST_DEPRECATED(effGetCurrentPosition),
    DECLARE_VST_DEPRECATED(effGetDestinationBuffer),
    effOfflineNotify,
    effOfflinePrepare,
    effOfflineRun,
    effProcessVarIo,
    effSetSpeakerArrangement,
    DECLARE_VST_DEPRECATED(effSetBlockSizeAndSampleRate),
    effSetBypass,
    effGetEffectName,
    DECLARE_VST_DEPRECATED(effGetErrorText),
    effGetVendorString,
    effGetProductString,
    effGetVendorVersion,
    effVendorSpecific,
    effCanDo,
    effGetTailSize,
    DECLARE_VST_DEPRECATED(effIdle),
    DECLARE_VST_DEPRECATED(effGetIcon),
    DECLARE_VST_DEPRECATED(effSetViewPosition),
    effGetParameterProperties,
    DECLARE_VST_DEPRECATED(effKeysRequired),
    effGetVstVersion,
    effEditKeyDown,
    effEditKeyUp,
    effSetEditKnobMode,
    effGetMidiProgramName,
    effGetCurrentMidiProgram,
    effGetMidiProgramCategory,
    effHasMidiProgramsChanged,
    effGetMidiKeyName,
    effBeginSetProgram,
    effEndSetProgram,
    effGetSpeakerArrangement,
    effShellGetNextPlugin,
    effStartProcess,
    effStopProcess,
    effSetTotalSampleToProcess,
    effSetPanLaw,
    effBeginLoadBank,
    effBeginLoadProgram,
    effSetProcessPrecision,
    effGetNumMidiInputChannels,
    effGetNumMidiOutputChannels
};

// ---- Audio master opcodes (plugin -> host) ----
enum
{
    audioMasterAutomate = 0,
    audioMasterVersion = 1,
    DECLARE_VST_DEPRECATED(audioMasterCurrentId),
    audioMasterIdle,
    DECLARE_VST_DEPRECATED(audioMasterPinConnected),
    audioMasterWantMidi = 6,
    audioMasterGetTime,
    audioMasterProcessEvents,
    DECLARE_VST_DEPRECATED(audioMasterSetTime),
    DECLARE_VST_DEPRECATED(audioMasterTempoAt),
    DECLARE_VST_DEPRECATED(audioMasterGetNumAutomatableParameters),
    DECLARE_VST_DEPRECATED(audioMasterGetParameterQuantization),
    audioMasterIOChanged,
    DECLARE_VST_DEPRECATED(audioMasterNeedIdle),
    audioMasterSizeWindow,
    audioMasterGetSampleRate,
    audioMasterGetBlockSize,
    audioMasterGetInputLatency,
    audioMasterGetOutputLatency,
    DECLARE_VST_DEPRECATED(audioMasterGetPreviousPlug),
    DECLARE_VST_DEPRECATED(audioMasterGetNextPlug),
    DECLARE_VST_DEPRECATED(audioMasterWillReplaceOrAccumulate),
    audioMasterGetCurrentProcessLevel,
    audioMasterGetAutomationState,
    audioMasterOfflineStart,
    audioMasterOfflineRead,
    audioMasterOfflineWrite,
    audioMasterOfflineGetCurrentPass,
    audioMasterOfflineGetCurrentMetaPass,
    DECLARE_VST_DEPRECATED(audioMasterSetOutputSampleRate),
    DECLARE_VST_DEPRECATED(audioMasterGetOutputSpeakerArrangement),
    audioMasterGetVendorString,
    audioMasterGetProductString,
    audioMasterGetVendorVersion,
    audioMasterVendorSpecific,
    DECLARE_VST_DEPRECATED(audioMasterSetIcon),
    audioMasterCanDo,
    audioMasterGetLanguage,
    DECLARE_VST_DEPRECATED(audioMasterOpenWindow),
    DECLARE_VST_DEPRECATED(audioMasterCloseWindow),
    audioMasterGetDirectory,
    audioMasterUpdateDisplay,
    audioMasterBeginEdit,
    audioMasterEndEdit,
    audioMasterOpenFileSelector,
    audioMasterCloseFileSelector,
    DECLARE_VST_DEPRECATED(audioMasterEditFile),
    DECLARE_VST_DEPRECATED(audioMasterGetChunkFile),
    DECLARE_VST_DEPRECATED(audioMasterGetInputSpeakerArrangement)
};

// ---- VstTimeInfo ----
enum VstTimeInfoFlags
{
    kVstTransportChanged     = 1,
    kVstTransportPlaying     = 1 << 1,
    kVstTransportCycleActive = 1 << 2,
    kVstTransportRecording   = 1 << 3,
    kVstAutomationWriting    = 1 << 6,
    kVstAutomationReading    = 1 << 7,
    kVstNanosValid           = 1 << 8,
    kVstPpqPosValid          = 1 << 9,
    kVstTempoValid           = 1 << 10,
    kVstBarsValid            = 1 << 11,
    kVstCyclePosValid        = 1 << 12,
    kVstTimeSigValid         = 1 << 13,
    kVstSmpteValid           = 1 << 14,
    kVstClockValid           = 1 << 15
};

#pragma pack(push, 8)
struct VstTimeInfo
{
    double samplePos;
    double sampleRate;
    double nanoSeconds;
    double ppqPos;
    double tempo;
    double barStartPos;
    double cycleStartPos;
    double cycleEndPos;
    int32_t timeSigNumerator;
    int32_t timeSigDenominator;
    int32_t smpteOffset;
    int32_t smpteFrameRate;
    int32_t samplesToNextClock;
    int32_t flags;
};
#pragma pack(pop)

// ---- SMPTE frame rates ----
enum VstSmpteFrameRate
{
    kVstSmpte24fps     = 0,
    kVstSmpte25fps     = 1,
    kVstSmpte2997fps   = 2,
    kVstSmpte30fps     = 3,
    kVstSmpte2997dfps  = 4,
    kVstSmpte30dfps    = 5,
    kVstSmpteFilm16mm  = 6,
    kVstSmpteFilm35mm  = 7,
    kVstSmpte239fps    = 10,
    kVstSmpte249fps    = 11,
    kVstSmpte599fps    = 12,
    kVstSmpte60fps     = 13
};

// ---- Events ----
enum VstEventTypes
{
    kVstMidiType  = 1,
    kVstSysExType = 6
};

#pragma pack(push, 8)
struct VstEvent
{
    int32_t type;
    int32_t byteSize;
    int32_t deltaFrames;
    int32_t flags;
    char data[16];
};

struct VstMidiEvent
{
    int32_t type;
    int32_t byteSize;
    int32_t deltaFrames;
    int32_t flags;
    int32_t noteLength;
    int32_t noteOffset;
    char midiData[4];
    char detune;
    char noteOffVelocity;
    char reserved1;
    char reserved2;
};

struct VstMidiSysexEvent
{
    int32_t type;
    int32_t byteSize;
    int32_t deltaFrames;
    int32_t flags;
    int32_t dumpBytes;
    VstIntPtr resvd1;
    char* sysexDump;
    VstIntPtr resvd2;
};

struct VstEvents
{
    int32_t numEvents;
    VstIntPtr reserved;
    VstEvent* events[2];
};
#pragma pack(pop)

// ---- Pin/parameter properties ----
enum VstPinPropertiesFlags
{
    kVstPinIsActive   = 1 << 0,
    kVstPinIsStereo   = 1 << 1,
    kVstPinUseSpeaker = 1 << 2
};

#pragma pack(push, 8)
struct VstPinProperties
{
    char label[kVstMaxLabelLen];
    int32_t flags;
    int32_t arrangementType;
    char shortLabel[kVstMaxShortLabelLen];
    char future[48];
};
#pragma pack(pop)

// ---- Plugin categories ----
enum VstPlugCategory
{
    kPlugCategUnknown = 0,
    kPlugCategEffect,
    kPlugCategSynth,
    kPlugCategAnalysis,
    kPlugCategMastering,
    kPlugCategSpacializer,
    kPlugCategRoomFx,
    kPlugSurroundFx,
    kPlugCategRestoration,
    kPlugCategOfflineProcess,
    kPlugCategShell,
    kPlugCategGenerator,
    kPlugCategMaxCount
};

// ---- Parameter properties ----
enum VstParameterFlags
{
    kVstParameterIsSwitch                = 1 << 0,
    kVstParameterUsesIntegerMinMax       = 1 << 1,
    kVstParameterUsesFloatStep           = 1 << 2,
    kVstParameterUsesIntStep             = 1 << 3,
    kVstParameterSupportsDisplayIndex    = 1 << 4,
    kVstParameterSupportsDisplayCategory = 1 << 5,
    kVstParameterCanRamp                 = 1 << 6
};

#pragma pack(push, 8)
struct VstParameterProperties
{
    float stepFloat;
    float smallStepFloat;
    float largeStepFloat;
    char label[kVstMaxLabelLen];
    int32_t flags;
    int32_t minInteger;
    int32_t maxInteger;
    int32_t stepInteger;
    int32_t largeStepInteger;
    char shortLabel[kVstMaxShortLabelLen];
    int16_t displayIndex;
    int16_t category;
    int16_t numParametersInCategory;
    int16_t reserved;
    char categoryLabel[kVstMaxCategLabelLen];
    char future[16];
};
#pragma pack(pop)

// ---- Speaker arrangement ----
enum VstSpeakerType
{
    kSpeakerUndefined = 0x7fffffff,
    kSpeakerM = 0,
    kSpeakerL,
    kSpeakerR,
    kSpeakerC,
    kSpeakerLfe,
    kSpeakerLs,
    kSpeakerRs,
    kSpeakerLc,
    kSpeakerRc,
    kSpeakerS,
    kSpeakerCs = kSpeakerS,
    kSpeakerSl,
    kSpeakerSr,
    kSpeakerTm,
    kSpeakerTfl,
    kSpeakerTfc,
    kSpeakerTfr,
    kSpeakerTrl,
    kSpeakerTrc,
    kSpeakerTrr,
    kSpeakerLfe2
};

enum VstSpeakerArrangementType
{
    kSpeakerArrUserDefined  = -2,
    kSpeakerArrEmpty        = -1,
    kSpeakerArrMono         = 0,
    kSpeakerArrStereo,
    kSpeakerArrStereoSurround,
    kSpeakerArrStereoCenter,
    kSpeakerArrStereoSide,
    kSpeakerArrStereoCLfe,
    kSpeakerArr30Cine,
    kSpeakerArr30Music,
    kSpeakerArr31Cine,
    kSpeakerArr31Music,
    kSpeakerArr40Cine,
    kSpeakerArr40Music,
    kSpeakerArr41Cine,
    kSpeakerArr41Music,
    kSpeakerArr50,
    kSpeakerArr51,
    kSpeakerArr60Cine,
    kSpeakerArr60Music,
    kSpeakerArr61Cine,
    kSpeakerArr61Music,
    kSpeakerArr70Cine,
    kSpeakerArr70Music,
    kSpeakerArr71Cine,
    kSpeakerArr71Music,
    kSpeakerArr80Cine,
    kSpeakerArr80Music,
    kSpeakerArr81Cine,
    kSpeakerArr81Music,
    kSpeakerArr102,
    kNumSpeakerArr
};

#pragma pack(push, 8)
struct VstSpeakerProperties
{
    float azimuth;
    float elevation;
    float radius;
    float reserved;
    char  name[kVstMaxNameLen];
    int32_t type;
    char future[28];
};

struct VstSpeakerArrangement
{
    int32_t type;
    int32_t numChannels;
    VstSpeakerProperties speakers[8];
};
#pragma pack(pop)

// ---- ERect for editor windows ----
#pragma pack(push, 8)
struct ERect
{
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
};
#pragma pack(pop)

// ---- MIDI programs ----
enum VstMidiProgramNameFlags
{
    kMidiIsOmni = 1
};

#pragma pack(push, 8)
struct MidiProgramName
{
    int32_t thisProgramIndex;
    char name[kVstMaxNameLen];
    int8_t midiProgram;
    int8_t midiBankMsb;
    int8_t midiBankLsb;
    int8_t reserved;
    int32_t parentCategoryIndex;
    int32_t flags;
};

struct MidiProgramCategory
{
    int32_t thisCategoryIndex;
    char name[kVstMaxNameLen];
    int32_t parentCategoryIndex;
    int32_t flags;
};

struct MidiKeyName
{
    int32_t thisProgramIndex;
    int32_t thisKeyNumber;
    char keyName[kVstMaxNameLen];
    int32_t reserved;
    int32_t flags;
};
#pragma pack(pop)

// ---- Process precision ----
enum VstProcessPrecision
{
    kVstProcessPrecision32 = 0,
    kVstProcessPrecision64
};

// ---- Pan law ----
enum VstPanLawType
{
    kLinearPanLaw = 0,
    kEqualPowerPanLaw
};

// ---- File selector ----
enum VstFileSelectCommand
{
    kVstFileLoad = 0,
    kVstFileSave,
    kVstMultipleFilesLoad,
    kVstDirectorySelect
};

enum VstFileSelectType
{
    kVstFileType = 0
};

#pragma pack(push, 8)
struct VstFileType
{
    char name[kVstMaxFileNameLen];
    char macType[8];
    char dosType[8];
    char unixType[8];
    char mimeType1[128];
    char mimeType2[128];
};

struct VstFileSelect
{
    int32_t command;
    int32_t type;
    int32_t macCreator;
    int32_t nbFileTypes;
    VstFileType* fileTypes;
    char title[1024];
    char* initialPath;
    char* returnPath;
    int32_t sizeReturnPath;
    char** returnMultiplePaths;
    int32_t nbReturnPath;
    VstIntPtr reserved;
    char future[116];
};
#pragma pack(pop)

// ---- Patch chunk info ----
#pragma pack(push, 8)
struct VstPatchChunkInfo
{
    int32_t version;
    int32_t pluginUniqueID;
    int32_t pluginVersion;
    int32_t numElements;
    char future[48];
};
#pragma pack(pop)

// ---- Key codes for editor key events ----
enum VstVirtualKey
{
    VKEY_BACK = 1,
    VKEY_TAB,
    VKEY_CLEAR,
    VKEY_RETURN,
    VKEY_PAUSE,
    VKEY_ESCAPE,
    VKEY_SPACE,
    VKEY_NEXT,
    VKEY_END,
    VKEY_HOME,
    VKEY_LEFT,
    VKEY_UP,
    VKEY_RIGHT,
    VKEY_DOWN,
    VKEY_PAGEUP,
    VKEY_PAGEDOWN,
    VKEY_SELECT,
    VKEY_PRINT,
    VKEY_ENTER,
    VKEY_SNAPSHOT,
    VKEY_INSERT,
    VKEY_DELETE,
    VKEY_HELP,
    VKEY_NUMPAD0,
    VKEY_NUMPAD1,
    VKEY_NUMPAD2,
    VKEY_NUMPAD3,
    VKEY_NUMPAD4,
    VKEY_NUMPAD5,
    VKEY_NUMPAD6,
    VKEY_NUMPAD7,
    VKEY_NUMPAD8,
    VKEY_NUMPAD9,
    VKEY_MULTIPLY,
    VKEY_ADD,
    VKEY_SEPARATOR,
    VKEY_SUBTRACT,
    VKEY_DECIMAL,
    VKEY_DIVIDE,
    VKEY_F1,
    VKEY_F2,
    VKEY_F3,
    VKEY_F4,
    VKEY_F5,
    VKEY_F6,
    VKEY_F7,
    VKEY_F8,
    VKEY_F9,
    VKEY_F10,
    VKEY_F11,
    VKEY_F12,
    VKEY_NUMLOCK,
    VKEY_SCROLL,
    VKEY_SHIFT,
    VKEY_CONTROL,
    VKEY_ALT,
    VKEY_EQUALS
};

enum VstModifierKey
{
    MODIFIER_SHIFT    = 1 << 0,
    MODIFIER_ALTERNATE = 1 << 1,
    MODIFIER_COMMAND  = 1 << 2,
    MODIFIER_CONTROL  = 1 << 3
};

#pragma pack(push, 8)
struct VstKeyCode
{
    int32_t character;
    unsigned char virt;
    unsigned char modifier;
};
#pragma pack(pop)

// ---- Knob modes ----
enum VstKnobMode
{
    kCircularMode = 0,
    kRelativCircularMode,
    kLinearMode
};

#endif // __aeffectx__
