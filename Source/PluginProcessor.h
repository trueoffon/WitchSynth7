#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Presets.h"
#include <array>
#include <atomic>
#include <set>
#include <vector>

class WitchSynthAudioProcessor : public juce::AudioProcessor
{
public:
    WitchSynthAudioProcessor();
    ~WitchSynthAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.5; }

    int getNumPrograms() override { return static_cast<int>(getAllPresets().size()); }
    int getCurrentProgram() override { return currentPresetIndex; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void loadPreset(int index);
    void initPatch();
    void triggerPanic();
    int getCurrentPresetIndex() const { return currentPresetIndex; }

    bool isFavorite(int presetIdx) const;
    void toggleFavorite(int presetIdx);
    std::vector<int> getFavoriteIndices() const;
    void saveFavorites();
    void loadFavorites();

    // --- Пользовательские пресеты (свои накрученные звуки) ---
    juce::File getUserPresetsDir() const;
    void saveUserPreset(const juce::String& name);
    juce::StringArray getUserPresetNames() const;
    bool loadUserPreset(const juce::String& name);

    std::atomic<bool> lockFx{ false };
    std::atomic<int> arpStepDisplay{ 0 };
    static constexpr int scopeSize = 256;
    std::array<float, scopeSize> scopeBuffer{};
    std::atomic<int> scopeWritePos{ 0 };
    std::atomic<float> peakLevel{ 0.0f };

    juce::MidiKeyboardState keyboardState;
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::File getFavoritesFile() const;

    struct Voice
    {
        static constexpr int maxUnison = 7;
        static constexpr int combSize = 4096;
        bool active = false;
        bool keyDown = false;
        int midiNote = -1;
        float velocity = 0.0f;
        float currentFreq = 440.0f;
        float targetFreq = 440.0f;
        float pitchEnvVal = 0.0f;
        float snapEnvVal = 0.0f;
        float stealGain = 1.0f;
        uint64_t age = 0;
        float entropyPitch = 0.0f;
        float entropyCutoff = 0.0f;

        std::array<float, maxUnison> phase1{};
        std::array<float, maxUnison> phase2{};
        float subPhase = 0.0f;
        float ghostPhase1 = 0.0f, ghostPhase2 = 0.0f;

        juce::ADSR ampEnv;
        juce::ADSR filterEnv;
        juce::dsp::StateVariableTPTFilter<float> filter;
        juce::dsp::StateVariableTPTFilter<float> formant2;

        std::array<float, combSize> combBufL{};
        std::array<float, combSize> combBufR{};
        int combPos = 0;
        float combLpL = 0.0f, combLpR = 0.0f;
        float combHpL = 0.0f, combHpR = 0.0f;
    };

    static constexpr int numVoices = 8;
    static constexpr int maxSubBlockSize = 4096;
    std::array<Voice, numVoices> voices;
    std::vector<int> heldNotes;
    bool sustainPedal = false;
    std::set<int> favoritePresets;
    uint64_t noteCounter = 0;

    float lfoPhase = 0.0f;
    float lfoSampleHold = 0.0f;
    double lorenzX = 0.1, lorenzY = 0.0, lorenzZ = 0.0;
    float pumpPhase = 0.0f;
    double arpPhase = 0.0;
    double prevHostPpq = -1.0;
    int lastArpStep = -1;
    int lastSubHit = 0;
    bool arpGateOpen = false;
    float arpGateSmooth = 1.0f;
    float pitchBendRatio = 1.0f;
    float tapeStopMult = 1.0f;

    uint32_t rngState = 0x9E3779B9u;
    float vhsWowPhase = 0.0f;
    float vhsSmoothNoise = 0.0f;
    float gravityEnergy = 0.0f;
    double currentSampleRate = 44100.0;
    int currentPresetIndex = 0;
    std::atomic<bool> resetFxFlag{ false };
    std::atomic<bool> panicFlag{ false };

    juce::SmoothedValue<float> smoothedCutoff, smoothedMaster, smoothedDelaySamples, smoothedMorphAmt;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Reverb reverb;
    juce::AudioBuffer<float> wetFxBuffer;
    juce::AudioBuffer<float> distDryBuffer;

    std::vector<float> delayBufL, delayBufR;
    int delayWritePos = 0;
    float delayFilterL = 0.0f, delayFilterR = 0.0f;

    static constexpr int ghostDelaySize = 2048;
    std::array<float, ghostDelaySize> ghostBufL{}, ghostBufR{};
    int ghostWritePos = 0;
    float ghostPh1 = 0.0f;

    static constexpr int shimmerSize = 4096;
    std::array<float, shimmerSize> shimmerBufL{}, shimmerBufR{};
    int shimmerWritePos = 0;
    float shimmerPh1 = 0.0f;
    float shimmerLpL = 0.0f, shimmerLpR = 0.0f;

    // --- Doubler (стерео-удвоитель с модулированной задержкой) ---
    static constexpr int doublerSize = 4096;
    std::array<float, doublerSize> doublerBufL{}, doublerBufR{};
    int doublerWritePos = 0;
    float doublerLfoPhase = 0.0f;

    float lowCutL1 = 0.0f, lowCutR1 = 0.0f;
    float fxHpL1 = 0.0f, fxHpL2 = 0.0f, fxHpR1 = 0.0f, fxHpR2 = 0.0f;
    float tiltLpL = 0.0f, tiltLpR = 0.0f;
    float airLpL = 0.0f, airLpR = 0.0f;
    float dcInL = 0.0f, dcOutL = 0.0f, dcInR = 0.0f, dcOutR = 0.0f;
    float crushHoldL = 0.0f, crushHoldR = 0.0f;
    int crushCounter = 0;

    void processSubBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int offset, int numSamples);
    void handleNoteOn(int note, float velocity, bool monoMode, bool arpActive, float glideTime);
    void handleNoteOff(int note, bool monoMode, bool arpActive);
    void releaseSustainedVoices();
    void renderAudioSegment(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, double bpm, bool hostPlaying, double hostPpq);

    static float fastSin(float phase01);
    static float polyBlep(float t, float dt);
    static float generateOsc(int waveType, float phase, float dt, float pw);
    float nextNoise();

    std::atomic<float> *pOsc1Wave, *pOsc1Oct, *pOsc1Semi, *pOsc1Detune, *pOsc1Level, *pPwmWidth, *pXModAmt;
    std::atomic<float> *pOsc2Wave, *pOsc2Oct, *pOsc2Semi, *pOsc2Unison, *pOsc2Level;
    std::atomic<float> *pSubLevel, *pEntropyAmt, *pGravityAmt, *pGhostVoxAmt;
    std::atomic<float> *pNoiseLevel, *pVhsDrift, *pCrushAmount, *pTiltAmt, *pLowCutHz;
    std::atomic<float> *pFilterType, *pFilterCutoff, *pFilterReso, *pFilterDrive, *pFilterEnvAmt, *pFilterDecay, *pFilterKeyTrack, *pPitchPunch;
    std::atomic<float> *pAmpA, *pAmpD, *pAmpS, *pAmpR, *pSnapAmt, *pAirAmt;
    std::atomic<float> *pLfoRate, *pLfoDepth, *pLfoTarget, *pLfoShape;
    std::atomic<float> *pPolyMode, *pArpMode, *pArpRate, *pGlideTime, *pStereoWidth, *pPumpAmt;
    std::array<std::atomic<float>*, 8> pSeq{};
    std::atomic<float> *pDistOn, *pDistMode, *pDistDrive, *pDistMix;
    std::atomic<float> *pChorusOn, *pChorusMix, *pChorusRate, *pChorusDepth;
    std::atomic<float> *pDelayOn, *pDelayMode, *pDelayTime, *pDelayFb, *pDelayMix;
    std::atomic<float> *pReverbOn, *pReverbFreeze, *pReverbSize, *pReverbMix, *pHaloAmt;
    std::atomic<float> *pMorphPreset, *pMorphAmt, *pTapeStop;
    std::atomic<float> *pMacroX, *pMacroY, *pMasterGain;
    std::atomic<float> *pDoublerOn, *pDoublerWidth, *pDoublerDtune, *pDoublerMix;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WitchSynthAudioProcessor)
};
