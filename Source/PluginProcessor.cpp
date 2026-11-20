#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

WitchSynthAudioProcessor::WitchSynthAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    heldNotes.reserve(32);
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);

    pOsc1Wave    = apvts.getRawParameterValue("osc1Wave");
    pOsc1Oct     = apvts.getRawParameterValue("osc1Oct");
    pOsc1Semi    = apvts.getRawParameterValue("osc1Semi");
    pOsc1Detune  = apvts.getRawParameterValue("osc1Detune");
    pOsc1Level   = apvts.getRawParameterValue("osc1Level");
    pPwmWidth    = apvts.getRawParameterValue("pwmWidth");
    pXModAmt     = apvts.getRawParameterValue("xModAmt");
    pOsc2Wave    = apvts.getRawParameterValue("osc2Wave");
    pOsc2Oct     = apvts.getRawParameterValue("osc2Oct");
    pOsc2Semi    = apvts.getRawParameterValue("osc2Semi");
    pOsc2Unison  = apvts.getRawParameterValue("osc2Unison");
    pOsc2Level   = apvts.getRawParameterValue("osc2Level");
    pSubLevel    = apvts.getRawParameterValue("subLevel");
    pEntropyAmt  = apvts.getRawParameterValue("entropyAmt");
    pGravityAmt  = apvts.getRawParameterValue("gravityAmt");
    pGhostVoxAmt = apvts.getRawParameterValue("ghostVoxAmt");
    pNoiseLevel  = apvts.getRawParameterValue("noiseLevel");
    pVhsDrift    = apvts.getRawParameterValue("vhsDrift");
    pCrushAmount = apvts.getRawParameterValue("crushAmount");
    pTiltAmt     = apvts.getRawParameterValue("tiltAmt");
    pLowCutHz    = apvts.getRawParameterValue("lowCutHz");
    pFilterType  = apvts.getRawParameterValue("filterType");
    pFilterCutoff= apvts.getRawParameterValue("filterCutoff");
    pFilterReso  = apvts.getRawParameterValue("filterReso");
    pFilterDrive = apvts.getRawParameterValue("filterDrive");
    pFilterEnvAmt= apvts.getRawParameterValue("filterEnvAmt");
    pFilterDecay = apvts.getRawParameterValue("filterDecay");
    pFilterKeyTrack = apvts.getRawParameterValue("filterKeyTrack");
    pPitchPunch  = apvts.getRawParameterValue("pitchPunch");
    pAmpA        = apvts.getRawParameterValue("ampAttack");
    pAmpD        = apvts.getRawParameterValue("ampDecay");
    pAmpS        = apvts.getRawParameterValue("ampSustain");
    pAmpR        = apvts.getRawParameterValue("ampRelease");
    pSnapAmt     = apvts.getRawParameterValue("snapAmt");
    pAirAmt      = apvts.getRawParameterValue("airAmt");
    pLfoRate     = apvts.getRawParameterValue("lfoRate");
    pLfoDepth    = apvts.getRawParameterValue("lfoDepth");
    pLfoTarget   = apvts.getRawParameterValue("lfoTarget");
    pLfoShape    = apvts.getRawParameterValue("lfoShape");
    pPolyMode    = apvts.getRawParameterValue("polyMode");
    pArpMode     = apvts.getRawParameterValue("arpMode");
    pArpRate     = apvts.getRawParameterValue("arpRate");
    pGlideTime   = apvts.getRawParameterValue("glideTime");
    pStereoWidth = apvts.getRawParameterValue("stereoWidth");
    pPumpAmt     = apvts.getRawParameterValue("pumpAmt");
    for (int i = 0; i < 8; ++i)
        pSeq[static_cast<size_t>(i)] = apvts.getRawParameterValue("seq" + juce::String(i));
    pDistOn      = apvts.getRawParameterValue("distOn");
    pDistMode    = apvts.getRawParameterValue("distMode");
    pDistDrive   = apvts.getRawParameterValue("distDrive");
    pDistMix     = apvts.getRawParameterValue("distMix");
    pChorusOn    = apvts.getRawParameterValue("chorusOn");
    pChorusMix   = apvts.getRawParameterValue("chorusMix");
    pChorusRate  = apvts.getRawParameterValue("chorusRate");
    pChorusDepth = apvts.getRawParameterValue("chorusDepth");
    pDelayOn     = apvts.getRawParameterValue("delayOn");
    pDelayMode   = apvts.getRawParameterValue("delayMode");
    pDelayTime   = apvts.getRawParameterValue("delayTime");
    pDelayFb     = apvts.getRawParameterValue("delayFeedback");
    pDelayMix    = apvts.getRawParameterValue("delayMix");
    pReverbOn    = apvts.getRawParameterValue("reverbOn");
    pReverbFreeze= apvts.getRawParameterValue("reverbFreeze");
    pReverbSize  = apvts.getRawParameterValue("reverbSize");
    pReverbMix   = apvts.getRawParameterValue("reverbMix");
    pHaloAmt     = apvts.getRawParameterValue("haloAmt");
    pMorphPreset = apvts.getRawParameterValue("morphPreset");
    pMorphAmt    = apvts.getRawParameterValue("morphAmt");
    pTapeStop    = apvts.getRawParameterValue("tapeStop");
    pMacroX      = apvts.getRawParameterValue("macroX");
    pMacroY      = apvts.getRawParameterValue("macroY");
    pMasterGain  = apvts.getRawParameterValue("masterGain");
    pDoublerOn    = apvts.getRawParameterValue("doublerOn");
    pDoublerWidth = apvts.getRawParameterValue("doublerWidth");
    pDoublerDtune = apvts.getRawParameterValue("doublerDtune");
    pDoublerMix   = apvts.getRawParameterValue("doublerMix");

    for (size_t i = 0; i < voices.size(); ++i)
    {
        voices[i].entropyPitch = (static_cast<float>(i) - 3.5f) * 0.0018f;
        voices[i].entropyCutoff = (static_cast<float>((i * 3) % 7) - 3.0f) * 0.05f;
    }

    loadFavorites();
    loadPreset(16);
}

juce::AudioProcessorValueTreeState::ParameterLayout WitchSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto pid = [](const char* s) { return juce::ParameterID{ s, 1 }; };
    juce::StringArray waves{ "Saw", "Pulse", "Tri", "NES4", "Sine", "Spectral", "ChaosPWM", "AcidForm" };

    p.push_back(std::make_unique<juce::AudioParameterChoice>(pid("osc1Wave"), "OSC1 Wave", waves, 1));
    p.push_back(std::make_unique<juce::AudioParameterInt>(pid("osc1Oct"), "OSC1 Oct", -2, 2, 0));
    p.push_back(std::make_unique<juce::AudioParameterInt>(pid("osc1Semi"), "OSC1 Semi", -12, 12, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("osc1Detune"), "Detune", 0.0f, 1.0f, 0.20f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("osc1Level"), "OSC1 Level", 0.0f, 1.0f, 0.90f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("pwmWidth"), "Morph/PWM", 0.08f, 0.92f, 0.28f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("xModAmt"), "FM/Sync", 0.0f, 1.0f, 0.05f));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(pid("osc2Wave"), "OSC2 Wave", waves, 3));
    p.push_back(std::make_unique<juce::AudioParameterInt>(pid("osc2Oct"), "OSC2 Oct", -2, 2, 0));
    p.push_back(std::make_unique<juce::AudioParameterInt>(pid("osc2Semi"), "OSC2 Semi", -12, 12, 0));
    p.push_back(std::make_unique<juce::AudioParameterInt>(pid("osc2Unison"), "Unison", 1, 7, 3));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("osc2Level"), "OSC2 Level", 0.0f, 1.0f, 0.65f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("subLevel"), "Sub/Trans", 0.0f, 1.0f, 0.30f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("entropyAmt"), "Entropy", 0.0f, 1.0f, 0.15f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("gravityAmt"), "Gravity", 0.0f, 1.0f, 0.05f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("ghostVoxAmt"), "Ghost Vox", 0.0f, 1.0f, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("noiseLevel"), "Noise", 0.0f, 1.0f, 0.02f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("vhsDrift"), "VHS Drift", 0.0f, 1.0f, 0.12f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("crushAmount"), "Crush", 0.0f, 1.0f, 0.10f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("tiltAmt"), "Tilt", -1.0f, 1.0f, 0.10f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("lowCutHz"), "Low Cut", juce::NormalisableRange<float>(20.0f, 250.0f, 1.0f, 0.45f), 24.0f));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(pid("filterType"), "Filter Type",
        juce::StringArray{ "LowPass", "HighPass", "BandPass", "Formant", "Comb", "DualAcid" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filterCutoff"), "Cutoff", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.25f), 6400.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filterReso"), "Resonance", juce::NormalisableRange<float>(0.5f, 12.0f, 0.01f, 0.5f), 2.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filterDrive"), "Filter Drive", 0.0f, 1.0f, 0.35f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filterEnvAmt"), "Filter Env", -1.0f, 1.0f, 0.25f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filterDecay"), "Filter Decay", juce::NormalisableRange<float>(0.01f, 4.0f, 0.001f, 0.35f), 0.35f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filterKeyTrack"), "Keytrack", 0.0f, 1.0f, 0.60f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("pitchPunch"), "Pitch Punch", 0.0f, 1.0f, 0.10f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("ampAttack"), "Attack", juce::NormalisableRange<float>(0.001f, 4.0f, 0.001f, 0.35f), 0.002f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("ampDecay"), "Decay", juce::NormalisableRange<float>(0.005f, 4.0f, 0.001f, 0.35f), 0.35f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("ampSustain"), "Sustain", 0.0f, 1.0f, 0.85f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("ampRelease"), "Release", juce::NormalisableRange<float>(0.005f, 6.0f, 0.001f, 0.35f), 0.25f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("snapAmt"), "Snap", 0.0f, 1.0f, 0.25f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("airAmt"), "Crystal Air", 0.0f, 1.0f, 0.45f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("lfoRate"), "LFO Rate", juce::NormalisableRange<float>(0.05f, 20.0f, 0.01f, 0.4f), 2.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("lfoDepth"), "LFO Depth", 0.0f, 1.0f, 0.20f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(pid("lfoTarget"), "LFO Target", juce::StringArray{ "Pitch", "Cutoff", "Morph" }, 2));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(pid("lfoShape"), "LFO Shape", juce::StringArray{ "Sine", "Square", "Ramp", "S&H", "BioChaos" }, 0));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(pid("polyMode"), "Voicing", juce::StringArray{ "Poly", "Mono" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(pid("arpMode"), "Arp/Chop", juce::StringArray{ "Off", "Stutter", "ChipArp", "Chaos" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(pid("arpRate"), "Arp Rate", juce::StringArray{ "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 3));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("glideTime"), "Glide", juce::NormalisableRange<float>(0.0f, 0.8f, 0.001f, 0.4f), 0.03f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("stereoWidth"), "Width", 0.0f, 1.0f, 0.70f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("pumpAmt"), "Auto Pump", 0.0f, 1.0f, 0.0f));

    for (int i = 0; i < 8; ++i)
        p.push_back(std::make_unique<juce::AudioParameterInt>(pid(("seq" + juce::String(i)).toRawUTF8()), "Seq " + juce::String(i), 0, 3, 1));

    p.push_back(std::make_unique<juce::AudioParameterBool>(pid("distOn"), "Dist On", true));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(pid("distMode"), "Dist Mode", juce::StringArray{ "Tube", "Diode", "Rectify", "FoldX", "BitClip" }, 1));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("distDrive"), "Dist Drive", 0.0f, 1.0f, 0.40f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("distMix"), "Dist Mix", 0.0f, 1.0f, 0.85f));

    p.push_back(std::make_unique<juce::AudioParameterBool>(pid("chorusOn"), "Chorus On", true));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("chorusMix"), "Chorus Mix", 0.0f, 1.0f, 0.35f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("chorusRate"), "Chorus Rate", juce::NormalisableRange<float>(0.1f, 8.0f, 0.01f, 0.5f), 1.1f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("chorusDepth"), "Chorus Depth", 0.0f, 1.0f, 0.38f));

    p.push_back(std::make_unique<juce::AudioParameterBool>(pid("delayOn"), "Delay On", true));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(pid("delayMode"), "Delay Mode", juce::StringArray{ "Stereo", "PingPong", "GhostOct" }, 1));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("delayTime"), "Delay Time", 0.05f, 1.0f, 0.22f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("delayFeedback"), "Delay FB", 0.0f, 0.92f, 0.44f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("delayMix"), "Delay Mix", 0.0f, 1.0f, 0.26f));

    p.push_back(std::make_unique<juce::AudioParameterBool>(pid("reverbOn"), "Reverb On", true));
    p.push_back(std::make_unique<juce::AudioParameterBool>(pid("reverbFreeze"), "Freeze", false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("reverbSize"), "Reverb Size", 0.0f, 1.0f, 0.65f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("reverbMix"), "Reverb Mix", 0.0f, 1.0f, 0.32f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("haloAmt"), "Shimmer Halo", 0.0f, 1.0f, 0.20f));

    p.push_back(std::make_unique<juce::AudioParameterInt>(pid("morphPreset"), "Morph Target B", 0, 199, 32));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("morphAmt"), "Morph A-B", 0.0f, 1.0f, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterBool>(pid("tapeStop"), "Tape Stop", false));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("macroX"), "Macro Chaos", 0.0f, 1.0f, 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("macroY"), "Macro Dimension", 0.0f, 1.0f, 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("masterGain"), "Master", 0.0f, 1.0f, 0.76f));

    p.push_back(std::make_unique<juce::AudioParameterBool>(pid("doublerOn"), "Doubler On", false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("doublerWidth"), "Doubler Width", 0.0f, 1.0f, 0.72f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("doublerDtune"), "Doubler Dtune", 0.0f, 30.0f, 14.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(pid("doublerMix"), "Doubler Mix", 0.0f, 1.0f, 0.55f));

    return { p.begin(), p.end() };
}

void WitchSynthAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    juce::dsp::ProcessSpec spec{ currentSampleRate, static_cast<juce::uint32>(maxSubBlockSize), 2 };

    keyboardState.reset();
    heldNotes.clear();
    sustainPedal = false;
    pitchBendRatio = 1.0f;
    for (auto& v : voices)
    {
        v.active = false;
        v.keyDown = false;
        v.pitchEnvVal = 0.0f;
        v.snapEnvVal = 0.0f;
        v.stealGain = 1.0f;
        v.ampEnv.setSampleRate(currentSampleRate);
        v.filterEnv.setSampleRate(currentSampleRate);
        v.filter.prepare(spec);
        v.filter.reset();
        v.formant2.prepare(spec);
        v.formant2.reset();
        v.combBufL.fill(0.0f);
        v.combBufR.fill(0.0f);
        v.combPos = 0;
        v.combLpL = v.combLpR = v.combHpL = v.combHpR = 0.0f;
    }

    oversampler->initProcessing(static_cast<size_t>(maxSubBlockSize));
    oversampler->reset();

    smoothedCutoff.reset(currentSampleRate, 0.012);
    smoothedMaster.reset(currentSampleRate, 0.012);
    smoothedDelaySamples.reset(currentSampleRate, 0.06);
    smoothedMorphAmt.reset(currentSampleRate, 0.02);
    smoothedCutoff.setCurrentAndTargetValue(pFilterCutoff->load());
    smoothedMaster.setCurrentAndTargetValue(pMasterGain->load());
    smoothedDelaySamples.setCurrentAndTargetValue(pDelayTime->load() * static_cast<float>(currentSampleRate));
    smoothedMorphAmt.setCurrentAndTargetValue(pMorphAmt->load());

    wetFxBuffer.setSize(2, maxSubBlockSize);
    wetFxBuffer.clear();
    distDryBuffer.setSize(2, maxSubBlockSize);
    distDryBuffer.clear();

    chorus.prepare(spec);
    chorus.setRate(1.1f);
    chorus.setDepth(0.38f);
    chorus.setCentreDelay(7.5f);
    chorus.setFeedback(0.22f);
    chorus.reset();

    reverb.prepare(spec);
    reverb.reset();

    const int maxDelaySamples = static_cast<int>(currentSampleRate * 2.0) + 16;
    delayBufL.assign(static_cast<size_t>(maxDelaySamples), 0.0f);
    delayBufR.assign(static_cast<size_t>(maxDelaySamples), 0.0f);
    delayWritePos = 0;
    delayFilterL = delayFilterR = 0.0f;

    ghostBufL.fill(0.0f);
    ghostBufR.fill(0.0f);
    ghostWritePos = 0;
    ghostPh1 = 0.0f;

    shimmerBufL.fill(0.0f);
    shimmerBufR.fill(0.0f);
    shimmerWritePos = 0;
    shimmerPh1 = 0.0f;
    shimmerLpL = shimmerLpR = 0.0f;

    doublerBufL.fill(0.0f);
    doublerBufR.fill(0.0f);
    doublerWritePos = 0;
    doublerLfoPhase = 0.0f;

    lowCutL1 = lowCutR1 = 0.0f;
    fxHpL1 = fxHpL2 = fxHpR1 = fxHpR2 = 0.0f;
    tiltLpL = tiltLpR = airLpL = airLpR = 0.0f;
    dcInL = dcOutL = dcInR = dcOutR = 0.0f;
    crushHoldL = crushHoldR = 0.0f;
    crushCounter = 0;
    arpPhase = 0.0;
    prevHostPpq = -1.0;
    lastArpStep = -1;
    lastSubHit = 0;
    arpGateSmooth = 1.0f;
    tapeStopMult = 1.0f;
    gravityEnergy = 0.0f;
    lorenzX = 0.1; lorenzY = 0.0; lorenzZ = 0.0;
}

void WitchSynthAudioProcessor::releaseResources() {}

bool WitchSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

float WitchSynthAudioProcessor::fastSin(float phase01)
{
    float x = phase01 - std::floor(phase01);
    x = x * 2.0f - 1.0f;
    const float y = 4.0f * (x - x * std::abs(x));
    return 0.775f * y + 0.225f * y * std::abs(y);
}

float WitchSynthAudioProcessor::polyBlep(float t, float dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

float WitchSynthAudioProcessor::generateOsc(int waveType, float phase, float dt, float pw)
{
    switch (waveType)
    {
        case 0:
            return (2.0f * phase - 1.0f) - polyBlep(phase, dt);
        case 1:
        {
            const float w = juce::jlimit(0.06f, 0.94f, pw);
            float s = phase < w ? 1.0f : -1.0f;
            s += polyBlep(phase, dt);
            float p2 = phase - w;
            if (p2 < 0.0f) p2 += 1.0f;
            s -= polyBlep(p2, dt);
            return s;
        }
        case 2:
            return phase < 0.5f ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
        case 3:
        {
            const float raw = phase < 0.5f ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
            return std::round(raw * 7.5f) / 7.5f;
        }
        case 4:
            return fastSin(phase);
        case 5:
        {
            const float s1 = fastSin(phase);
            const float s2 = fastSin(phase * 2.0f);
            const float s3 = fastSin(phase * 3.0f);
            const float s5 = fastSin(phase * 5.0f);
            const float s7 = fastSin(phase * 7.0f);
            return 0.52f * s1 + pw * 0.34f * s2 + 0.26f * s3 + (1.0f - pw) * 0.24f * s5 + pw * 0.20f * s7;
        }
        case 6:
        {
            const float w1 = juce::jlimit(0.05f, 0.45f, pw * 0.45f);
            const float w2 = juce::jlimit(0.05f, 0.40f, (1.0f - pw) * 0.38f);
            float s = (phase < w1 || (phase > 0.5f && phase < 0.5f + w2)) ? 1.0f : -1.0f;
            s += polyBlep(phase, dt);
            return s;
        }
        default:
        {
            const float mult = 1.5f + pw * 4.5f;
            return fastSin(phase * mult) * std::exp(-phase * 3.2f) * 1.35f;
        }
    }
}

float WitchSynthAudioProcessor::nextNoise()
{
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;
    return static_cast<float>(static_cast<int32_t>(rngState)) * (1.0f / 2147483648.0f);
}

void WitchSynthAudioProcessor::handleNoteOn(int note, float velocity, bool monoMode, bool arpActive, float glideTime)
{
    note = juce::jlimit(0, 127, note);
    velocity = juce::jlimit(0.0f, 1.0f, velocity);
    const bool wasEmpty = heldNotes.empty();
    heldNotes.erase(std::remove(heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
    heldNotes.push_back(note);

    const float freq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(note));

    if (arpActive)
    {
        for (int i = 1; i < numVoices; ++i) { voices[i].active = false; voices[i].keyDown = false; }
        auto& v = voices[0];
        if (wasEmpty || !v.active)
        {
            v.active = true;
            v.keyDown = true;
            v.midiNote = note;
            v.velocity = velocity;
            v.currentFreq = freq;
            v.targetFreq = freq;
            v.pitchEnvVal = 1.0f;
            v.snapEnvVal = 1.0f;
            v.stealGain = 1.0f;
            lastArpStep = -1;
            arpPhase = 0.0;
        }
        return;
    }

    if (monoMode)
    {
        auto& v = voices[0];
        const bool envAlive = v.active && v.ampEnv.isActive();
        const bool retrigger = wasEmpty || !envAlive || (pAmpS->load() < 0.25f);

        v.active = true;
        v.keyDown = true;
        v.midiNote = note;
        v.velocity = velocity;
        v.targetFreq = freq;

        if (!envAlive)
        {
            v.currentFreq = freq;
            for (int u = 0; u < Voice::maxUnison; ++u)
            {
                v.phase1[static_cast<size_t>(u)] = std::fmod(static_cast<float>(u) * 0.6180339f, 1.0f);
                v.phase2[static_cast<size_t>(u)] = std::fmod(static_cast<float>(u) * 0.3819660f + 0.25f, 1.0f);
            }
            v.subPhase = 0.0f;
            v.ghostPhase1 = v.ghostPhase2 = 0.0f;
            v.filter.reset();
            v.formant2.reset();
            v.combBufL.fill(0.0f);
            v.combBufR.fill(0.0f);
        }
        else if (wasEmpty && glideTime <= 0.001f)
        {
            v.currentFreq = freq;
        }

        if (retrigger)
        {
            v.pitchEnvVal = 1.0f;
            v.snapEnvVal = 1.0f;
            v.stealGain = 1.0f;
            v.ampEnv.noteOn();
            v.filterEnv.noteOn();
        }
        return;
    }

    int targetIdx = -1;
    for (int i = 0; i < numVoices; ++i)
        if (!voices[i].active) { targetIdx = i; break; }

    bool stolen = false;
    if (targetIdx < 0)
    {
        uint64_t oldestRel = UINT64_MAX;
        for (int i = 0; i < numVoices; ++i)
        {
            if (!voices[i].keyDown && voices[i].age < oldestRel)
            {
                oldestRel = voices[i].age;
                targetIdx = i;
            }
        }
        if (targetIdx < 0)
        {
            uint64_t oldest = UINT64_MAX;
            for (int i = 0; i < numVoices; ++i)
                if (voices[i].age < oldest) { oldest = voices[i].age; targetIdx = i; }
        }
        stolen = true;
    }

    auto& v = voices[targetIdx];
    v.active = true;
    v.keyDown = true;
    v.midiNote = note;
    v.velocity = velocity;
    v.currentFreq = freq;
    v.targetFreq = freq;
    v.pitchEnvVal = 1.0f;
    v.snapEnvVal = 1.0f;
    v.stealGain = stolen ? 0.0f : 1.0f;
    v.age = ++noteCounter;
    for (int u = 0; u < Voice::maxUnison; ++u)
    {
        v.phase1[static_cast<size_t>(u)] = std::fmod(static_cast<float>(u) * 0.6180339f, 1.0f);
        v.phase2[static_cast<size_t>(u)] = std::fmod(static_cast<float>(u) * 0.3819660f + 0.25f, 1.0f);
    }
    v.subPhase = 0.0f;
    v.ghostPhase1 = v.ghostPhase2 = 0.0f;
    v.filter.reset();
    v.formant2.reset();
    v.combBufL.fill(0.0f);
    v.combBufR.fill(0.0f);
    v.ampEnv.noteOn();
    v.filterEnv.noteOn();
}

void WitchSynthAudioProcessor::releaseSustainedVoices()
{
    if (sustainPedal) return;
    for (auto& v : voices)
        if (v.active && !v.keyDown) { v.ampEnv.noteOff(); v.filterEnv.noteOff(); }
}

void WitchSynthAudioProcessor::handleNoteOff(int note, bool monoMode, bool arpActive)
{
    note = juce::jlimit(0, 127, note);
    heldNotes.erase(std::remove(heldNotes.begin(), heldNotes.end(), note), heldNotes.end());

    if (arpActive)
    {
        if (heldNotes.empty() && voices[0].active)
        {
            voices[0].keyDown = false;
            if (!sustainPedal)
            {
                voices[0].ampEnv.noteOff();
                voices[0].filterEnv.noteOff();
            }
        }
        return;
    }

    if (monoMode)
    {
        auto& v = voices[0];
        if (!heldNotes.empty())
        {
            const int prevNote = heldNotes.back();
            v.midiNote = prevNote;
            v.targetFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(prevNote));
        }
        else if (v.active)
        {
            v.keyDown = false;
            if (!sustainPedal)
            {
                v.ampEnv.noteOff();
                v.filterEnv.noteOff();
            }
        }
        return;
    }

    int oldestMatch = -1;
    uint64_t minAge = UINT64_MAX;
    for (int i = 0; i < numVoices; ++i)
    {
        if (voices[i].active && voices[i].keyDown && voices[i].midiNote == note && voices[i].age < minAge)
        {
            minAge = voices[i].age;
            oldestMatch = i;
        }
    }
    if (oldestMatch >= 0)
    {
        voices[oldestMatch].keyDown = false;
        if (!sustainPedal)
        {
            voices[oldestMatch].ampEnv.noteOff();
            voices[oldestMatch].filterEnv.noteOff();
        }
    }
}

void WitchSynthAudioProcessor::triggerPanic() { panicFlag.store(true); }

void WitchSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (buffer.getNumChannels() < 2) return;

    const int totalSamples = buffer.getNumSamples();
    for (int channel = 2; channel < buffer.getNumChannels(); ++channel)
        buffer.clear(channel, 0, totalSamples);
    if (totalSamples <= 0) return;

    keyboardState.processNextMidiBuffer(midiMessages, 0, totalSamples, true);

    for (int offset = 0; offset < totalSamples; offset += maxSubBlockSize)
    {
        const int chunk = juce::jmin(maxSubBlockSize, totalSamples - offset);
        processSubBlock(buffer, midiMessages, offset, chunk);
    }
}

void WitchSynthAudioProcessor::processSubBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, int offset, int nSamples)
{
    if (panicFlag.exchange(false))
    {
        heldNotes.clear();
        sustainPedal = false;
        pitchBendRatio = 1.0f;
        for (auto& v : voices) { v.active = false; v.keyDown = false; v.ampEnv.reset(); v.filterEnv.reset(); v.combBufL.fill(0.0f); v.combBufR.fill(0.0f); }
        resetFxFlag.store(true);
    }

    if (resetFxFlag.exchange(false))
    {
        std::fill(delayBufL.begin(), delayBufL.end(), 0.0f);
        std::fill(delayBufR.begin(), delayBufR.end(), 0.0f);
        ghostBufL.fill(0.0f); ghostBufR.fill(0.0f);
        shimmerBufL.fill(0.0f); shimmerBufR.fill(0.0f);
        doublerBufL.fill(0.0f); doublerBufR.fill(0.0f);
        delayFilterL = delayFilterR = shimmerLpL = shimmerLpR = fxHpL1 = fxHpL2 = fxHpR1 = fxHpR2 = lowCutL1 = lowCutR1 = 0.0f;
        chorus.reset();
        reverb.reset();
        oversampler->reset();
    }

    double bpm = 128.0;
    double hostPpq = 0.0;
    bool hostPlaying = false;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto p = pos->getPpqPosition()) hostPpq = *p;
            hostPlaying = pos->getIsPlaying();
        }
    }

    if (hostPlaying && prevHostPpq >= 0.0 && hostPpq < prevHostPpq - 0.05)
        lastArpStep = -1;
    prevHostPpq = hostPlaying ? hostPpq : -1.0;

    const bool monoMode = pPolyMode->load() > 0.5f;
    const int arpMode = static_cast<int>(pArpMode->load());
    const bool arpActive = arpMode >= 2;
    const float glideTime = pGlideTime->load();

    auto it = midiMessages.cbegin();
    const auto endIt = midiMessages.cend();
    while (it != endIt && (*it).samplePosition < offset) ++it;

    int currentLocal = 0;
    const int endOffset = offset + nSamples;
    while (it != endIt && (*it).samplePosition < endOffset)
    {
        const int eventPos = (*it).samplePosition - offset;
        if (eventPos > currentLocal)
        {
            renderAudioSegment(buffer, offset + currentLocal, eventPos - currentLocal, bpm, hostPlaying, hostPpq);
            currentLocal = eventPos;
        }

        auto groupEnd = it;
        while (groupEnd != endIt && ((*groupEnd).samplePosition - offset) == eventPos)
            ++groupEnd;

        for (auto mIt = it; mIt != groupEnd; ++mIt)
        {
            const auto msg = (*mIt).getMessage();
            if (msg.isNoteOff())
                handleNoteOff(msg.getNoteNumber(), monoMode, arpActive);
            else if (msg.isController() && msg.getControllerNumber() == 64)
            {
                sustainPedal = msg.getControllerValue() >= 64;
                if (!sustainPedal) releaseSustainedVoices();
            }
            else if (msg.isPitchWheel())
            {
                const float norm = (static_cast<float>(msg.getPitchWheelValue()) - 8192.0f) / 8192.0f;
                pitchBendRatio = std::pow(2.0f, norm * (2.0f / 12.0f));
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                heldNotes.clear();
                sustainPedal = false;
                for (auto& v : voices) { v.keyDown = false; v.ampEnv.noteOff(); v.filterEnv.noteOff(); }
            }
        }
        for (auto mIt = it; mIt != groupEnd; ++mIt)
        {
            const auto msg = (*mIt).getMessage();
            if (msg.isNoteOn())
                handleNoteOn(msg.getNoteNumber(), msg.getFloatVelocity(), monoMode, arpActive, glideTime);
        }

        it = groupEnd;
    }

    if (currentLocal < nSamples)
        renderAudioSegment(buffer, offset + currentLocal, nSamples - currentLocal, bpm, hostPlaying, hostPpq);

    auto* L = buffer.getWritePointer(0) + offset;
    auto* R = buffer.getWritePointer(1) + offset;

    const auto& allPresets = getAllPresets();
    const int morphIdx = juce::jlimit(0, static_cast<int>(allPresets.size()) - 1, static_cast<int>(pMorphPreset->load()));
    const auto& targetB = allPresets[static_cast<size_t>(morphIdx)];
    const float mAmt = smoothedMorphAmt.getCurrentValue();

    const float macroX = (pMacroX->load() - 0.5f) * 2.0f;
    const float macroY = (pMacroY->load() - 0.5f) * 2.0f;

    const float baseDist = juce::jmap(mAmt, pDistDrive->load(), targetB.distDrive);
    const float effDistDrive = juce::jlimit(0.0f, 1.0f, baseDist + macroX * 0.25f);
    const float distMix = pDistMix->load();

    juce::dsp::AudioBlock<float> fullBufBlock(buffer);
    auto subBlock = fullBufBlock.getSubBlock(static_cast<size_t>(offset), static_cast<size_t>(nSamples));

    if (pDistOn->load() > 0.5f && effDistDrive > 0.01f && distMix > 0.01f)
    {
        distDryBuffer.copyFrom(0, 0, L, nSamples);
        distDryBuffer.copyFrom(1, 0, R, nSamples);

        const int dMode = static_cast<int>(pDistMode->load());
        const float drive = 1.0f + effDistDrive * 15.0f;
        const float norm = 1.0f / std::tanh(drive);

        auto osBlock = oversampler->processSamplesUp(subBlock);
        const size_t osN = osBlock.getNumSamples();
        auto* osL = osBlock.getChannelPointer(0);
        auto* osR = osBlock.getChannelPointer(1);

        auto shapeSample = [&](float x) -> float
        {
            if (dMode == 0) return std::tanh(x * drive) * norm;
            if (dMode == 1)
            {
                const float d = x * drive * 0.85f;
                return d >= 0.0f ? std::tanh(d * 1.45f) : std::tanh(d * 0.72f) * 0.82f;
            }
            if (dMode == 2)
            {
                const float d = x * drive * 0.55f;
                return juce::jlimit(-1.0f, 1.0f, std::sin((std::abs(d) * 0.65f + d * 0.35f) * 2.1f));
            }
            if (dMode == 3)
            {
                const float d = x * (1.0f + effDistDrive * 6.5f);
                return std::sin(d * juce::MathConstants<float>::halfPi + 0.45f * std::sin(d * 3.14f)) * 0.92f;
            }
            const float d = std::tanh(x * drive * 0.95f);
            return std::round(d * 12.0f) / 12.0f;
        };

        for (size_t i = 0; i < osN; ++i)
        {
            osL[i] = shapeSample(osL[i]);
            osR[i] = shapeSample(osR[i]);
        }
        oversampler->processSamplesDown(subBlock);

        const auto* dryL = distDryBuffer.getReadPointer(0);
        const auto* dryR = distDryBuffer.getReadPointer(1);
        for (int i = 0; i < nSamples; ++i)
        {
            L[i] = dryL[i] * (1.0f - distMix) + L[i] * distMix;
            R[i] = dryR[i] * (1.0f - distMix) + R[i] * distMix;
        }
    }

    const float baseCrush = juce::jmap(mAmt, pCrushAmount->load(), targetB.crushAmount);
    const float crush = juce::jlimit(0.0f, 1.0f, baseCrush + macroX * 0.18f);
    if (crush > 0.01f)
    {
        const int holdSamples = 1 + static_cast<int>(crush * 22.0f);
        const float levels = std::pow(2.0f, 14.0f - crush * 10.5f);
        for (int i = 0; i < nSamples; ++i)
        {
            if (crushCounter++ >= holdSamples)
            {
                crushCounter = 0;
                crushHoldL = std::round(L[i] * levels) / levels;
                crushHoldR = std::round(R[i] * levels) / levels;
            }
            L[i] = L[i] * (1.0f - crush * 0.85f) + crushHoldL * (crush * 0.85f);
            R[i] = R[i] * (1.0f - crush * 0.85f) + crushHoldR * (crush * 0.85f);
        }
    }

    const float tilt = juce::jlimit(-1.0f, 1.0f, juce::jmap(mAmt, pTiltAmt->load(), targetB.tiltAmt));
    if (std::abs(tilt) > 0.01f)
    {
        const float tAlpha = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 1000.0f / static_cast<float>(currentSampleRate));
        for (int i = 0; i < nSamples; ++i)
        {
            tiltLpL += tAlpha * (L[i] - tiltLpL);
            tiltLpR += tAlpha * (R[i] - tiltLpR);
            const float hiL = L[i] - tiltLpL, hiR = R[i] - tiltLpR;
            L[i] = tiltLpL * (1.0f - tilt * 0.45f) + hiL * (1.0f + tilt * 0.65f);
            R[i] = tiltLpR * (1.0f - tilt * 0.45f) + hiR * (1.0f + tilt * 0.65f);
        }
    }

    const float lowCutHz = pLowCutHz->load();
    if (lowCutHz > 22.0f)
    {
        const float lcAlpha = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * lowCutHz / static_cast<float>(currentSampleRate));
        for (int i = 0; i < nSamples; ++i)
        {
            lowCutL1 += lcAlpha * (L[i] - lowCutL1);
            lowCutR1 += lcAlpha * (R[i] - lowCutR1);
            L[i] -= lowCutL1;
            R[i] -= lowCutR1;
        }
    }

    const float pumpAmt = juce::jmap(mAmt, pPumpAmt->load(), targetB.pumpAmt);
    if (pumpAmt > 0.01f)
    {
        const float beatInc = static_cast<float>(bpm / (60.0 * currentSampleRate));
        float curBeat = hostPlaying ? static_cast<float>(std::fmod(std::max(0.0, hostPpq), 1.0)) : pumpPhase;
        for (int i = 0; i < nSamples; ++i)
        {
            const float ramp = juce::jmin(1.0f, curBeat * 45.0f);
            const float duck = 1.0f - pumpAmt * std::exp(-curBeat * 4.5f) * ramp;
            L[i] *= duck;
            R[i] *= duck;
            curBeat += beatInc;
            if (curBeat >= 1.0f) curBeat -= 1.0f;
        }
        pumpPhase = curBeat;
    }

    const float width = juce::jlimit(0.0f, 1.0f, pStereoWidth->load() + macroY * 0.25f) * 2.0f;
    for (int i = 0; i < nSamples; ++i)
    {
        const float mid = 0.5f * (L[i] + R[i]);
        const float side = 0.5f * (L[i] - R[i]) * width;
        L[i] = mid + side;
        R[i] = mid - side;
    }

    if (pChorusOn->load() > 0.5f)
    {
        juce::dsp::ProcessContextReplacing<float> dryCtx(subBlock);
        chorus.setRate(pChorusRate->load());
        chorus.setDepth(pChorusDepth->load());
        chorus.setMix(juce::jlimit(0.0f, 1.0f, pChorusMix->load() + macroY * 0.15f));
        chorus.process(dryCtx);
    }

    // --- DOUBLER: стерео-удвоитель ---
    if (pDoublerOn->load() > 0.5f)
    {
        const float dblMix   = juce::jlimit(0.0f, 1.0f, pDoublerMix->load());
        const float dblWidth = juce::jlimit(0.0f, 1.0f, pDoublerWidth->load());
        const float dblDtune = juce::jlimit(0.0f, 30.0f, pDoublerDtune->load());
        const int   dblLen   = doublerSize;

        // базовая задержка ~14 мс, модуляция времени даёт расстройку
        const float baseDelay = 0.014f * static_cast<float>(currentSampleRate);
        const float modDepth  = dblDtune * 0.55f;                 // в сэмплах
        const float lfoInc    = 0.6f / static_cast<float>(currentSampleRate); // ~0.6 Гц

        for (int i = 0; i < nSamples; ++i)
        {
            doublerBufL[static_cast<size_t>(doublerWritePos)] = L[i];
            doublerBufR[static_cast<size_t>(doublerWritePos)] = R[i];

            doublerLfoPhase += lfoInc;
            if (doublerLfoPhase >= 1.0f) doublerLfoPhase -= 1.0f;
            const float lfoL = std::sin(doublerLfoPhase * juce::MathConstants<float>::twoPi);
            const float lfoR = std::sin((doublerLfoPhase + 0.5f) * juce::MathConstants<float>::twoPi);

            const float dSampL = juce::jlimit(1.0f, static_cast<float>(dblLen - 2), baseDelay + lfoL * modDepth);
            const float dSampR = juce::jlimit(1.0f, static_cast<float>(dblLen - 2), baseDelay * 1.18f + lfoR * modDepth);

            auto readTap = [&](const std::array<float, doublerSize>& buf, float dSamp) -> float
            {
                const int di = static_cast<int>(dSamp);
                const float fr = dSamp - static_cast<float>(di);
                const int a = (doublerWritePos - di + dblLen) % dblLen;
                const int b = (doublerWritePos - di - 1 + dblLen) % dblLen;
                return buf[static_cast<size_t>(a)] * (1.0f - fr) + buf[static_cast<size_t>(b)] * fr;
            };

            // крест-накрест: копия левого уходит вправо и наоборот -> ширина
            const float copyL = readTap(doublerBufR, dSampL) * dblWidth;
            const float copyR = readTap(doublerBufL, dSampR) * dblWidth;

            L[i] = L[i] * (1.0f - dblMix) + (L[i] + copyL) * dblMix * 0.5f;
            R[i] = R[i] * (1.0f - dblMix) + (R[i] + copyR) * dblMix * 0.5f;

            doublerWritePos = (doublerWritePos + 1) % dblLen;
        }
    }

    auto* wL = wetFxBuffer.getWritePointer(0);
    auto* wR = wetFxBuffer.getWritePointer(1);
    const float hpAlpha = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 180.0f / static_cast<float>(currentSampleRate));
    for (int i = 0; i < nSamples; ++i)
    {
        fxHpL1 += hpAlpha * (L[i] - fxHpL1);
        const float hp1L = L[i] - fxHpL1;
        fxHpL2 += hpAlpha * (hp1L - fxHpL2);
        wL[i] = hp1L - fxHpL2;

        fxHpR1 += hpAlpha * (R[i] - fxHpR1);
        const float hp1R = R[i] - fxHpR1;
        fxHpR2 += hpAlpha * (hp1R - fxHpR2);
        wR[i] = hp1R - fxHpR2;
    }

    const float effDelayMix = juce::jlimit(0.0f, 1.0f, pDelayMix->load() + macroY * 0.22f);
    if (pDelayOn->load() > 0.5f && !delayBufL.empty() && effDelayMix > 0.005f)
    {
        const int bufLen = static_cast<int>(delayBufL.size());
        const int dMode = static_cast<int>(pDelayMode->load());
        smoothedDelaySamples.setTargetValue(pDelayTime->load() * static_cast<float>(currentSampleRate));
        const float fb = pDelayFb->load();
        constexpr int gW = ghostDelaySize / 2;
        constexpr float gInc = 1.0f / static_cast<float>(gW);

        for (int i = 0; i < nSamples; ++i)
        {
            const float dSamp = juce::jlimit(1.0f, static_cast<float>(bufLen - 2), smoothedDelaySamples.getNextValue());
            const int dInt = static_cast<int>(dSamp);
            const float frac = dSamp - static_cast<float>(dInt);
            const int p0 = (delayWritePos - dInt + bufLen) % bufLen;
            const int p1 = (delayWritePos - dInt - 1 + bufLen) % bufLen;

            const float dL = delayBufL[static_cast<size_t>(p0)] * (1.0f - frac) + delayBufL[static_cast<size_t>(p1)] * frac;
            const float dR = delayBufR[static_cast<size_t>(p0)] * (1.0f - frac) + delayBufR[static_cast<size_t>(p1)] * frac;

            delayFilterL += 0.28f * (dL - delayFilterL);
            delayFilterR += 0.28f * (dR - delayFilterR);

            float fbL = std::tanh(delayFilterL * fb);
            float fbR = std::tanh(delayFilterR * fb);

            if (dMode == 2)
            {
                ghostBufL[static_cast<size_t>(ghostWritePos)] = fbL;
                ghostBufR[static_cast<size_t>(ghostWritePos)] = fbR;
                const float ph2 = ghostPh1 >= 0.5f ? (ghostPh1 - 0.5f) : (ghostPh1 + 0.5f);
                const int r1 = (ghostWritePos - static_cast<int>((1.0f - ghostPh1) * static_cast<float>(gW)) + ghostDelaySize) % ghostDelaySize;
                const int r2 = (ghostWritePos - static_cast<int>((1.0f - ph2) * static_cast<float>(gW)) + ghostDelaySize) % ghostDelaySize;
                const float wt1 = 1.0f - std::abs(2.0f * ghostPh1 - 1.0f);
                fbL = ghostBufL[static_cast<size_t>(r1)] * wt1 + ghostBufL[static_cast<size_t>(r2)] * (1.0f - wt1);
                fbR = ghostBufR[static_cast<size_t>(r1)] * wt1 + ghostBufR[static_cast<size_t>(r2)] * (1.0f - wt1);
                ghostWritePos = (ghostWritePos + 1) % ghostDelaySize;
                ghostPh1 += gInc;
                if (ghostPh1 >= 1.0f) ghostPh1 -= 1.0f;
            }

            if (dMode == 0)
            {
                delayBufL[static_cast<size_t>(delayWritePos)] = wL[i] + fbL;
                delayBufR[static_cast<size_t>(delayWritePos)] = wR[i] + fbR;
            }
            else
            {
                delayBufL[static_cast<size_t>(delayWritePos)] = wL[i] + fbR;
                delayBufR[static_cast<size_t>(delayWritePos)] = wR[i] * 0.5f + fbL;
            }

            L[i] += delayFilterL * effDelayMix;
            R[i] += delayFilterR * effDelayMix;
            wL[i] += delayFilterL * effDelayMix * 0.45f;
            wR[i] += delayFilterR * effDelayMix * 0.45f;
            delayWritePos = (delayWritePos + 1) % bufLen;
        }
    }

    const float baseHalo = juce::jmap(mAmt, pHaloAmt->load(), targetB.haloAmt);
    const float effRevMix = juce::jlimit(0.0f, 1.0f, pReverbMix->load() + macroY * 0.35f);
    const float effHalo = juce::jlimit(0.0f, 1.0f, baseHalo + macroY * 0.25f);
    const bool freezeOn = pReverbFreeze->load() > 0.5f;
    if (pReverbOn->load() > 0.5f && (effRevMix > 0.005f || freezeOn))
    {
        if (effHalo > 0.01f && !freezeOn)
        {
            constexpr int W = shimmerSize / 2;
            constexpr float phInc = 1.0f / static_cast<float>(W);
            for (int i = 0; i < nSamples; ++i)
            {
                shimmerBufL[static_cast<size_t>(shimmerWritePos)] = wL[i];
                shimmerBufR[static_cast<size_t>(shimmerWritePos)] = wR[i];
                const float ph2 = shimmerPh1 >= 0.5f ? (shimmerPh1 - 0.5f) : (shimmerPh1 + 0.5f);
                const int r1 = (shimmerWritePos - static_cast<int>((1.0f - shimmerPh1) * static_cast<float>(W)) + shimmerSize) % shimmerSize;
                const int r2 = (shimmerWritePos - static_cast<int>((1.0f - ph2) * static_cast<float>(W)) + shimmerSize) % shimmerSize;
                const float wt1 = 1.0f - std::abs(2.0f * shimmerPh1 - 1.0f);
                const float wt2 = 1.0f - wt1;

                const float octL = shimmerBufL[static_cast<size_t>(r1)] * wt1 + shimmerBufL[static_cast<size_t>(r2)] * wt2;
                const float octR = shimmerBufR[static_cast<size_t>(r1)] * wt1 + shimmerBufR[static_cast<size_t>(r2)] * wt2;
                shimmerLpL += 0.22f * (octL - shimmerLpL);
                shimmerLpR += 0.22f * (octR - shimmerLpR);
                wL[i] += shimmerLpR * effHalo * 0.42f;
                wR[i] += shimmerLpL * effHalo * 0.42f;

                shimmerWritePos = (shimmerWritePos + 1) % shimmerSize;
                shimmerPh1 += phInc;
                if (shimmerPh1 >= 1.0f) shimmerPh1 -= 1.0f;
            }
        }

        juce::dsp::Reverb::Parameters rp;
        rp.roomSize = juce::jlimit(0.0f, 1.0f, pReverbSize->load() + macroY * 0.2f);
        rp.damping = 0.68f;
        rp.wetLevel = (freezeOn ? juce::jmax(0.45f, effRevMix) : effRevMix) * 0.72f;
        rp.dryLevel = 0.0f;
        rp.width = 1.0f;
        rp.freezeMode = freezeOn ? 1.0f : 0.0f;
        reverb.setParameters(rp);

        juce::dsp::AudioBlock<float> wetFullBlock(wetFxBuffer);
        auto wetSubBlock = wetFullBlock.getSubBlock(0, static_cast<size_t>(nSamples));
        juce::dsp::ProcessContextReplacing<float> wetCtx(wetSubBlock);
        reverb.process(wetCtx);

        for (int i = 0; i < nSamples; ++i)
        {
            L[i] = L[i] * (1.0f - effRevMix * 0.20f) + wL[i];
            R[i] = R[i] * (1.0f - effRevMix * 0.20f) + wR[i];
        }
    }

    const float air = juce::jlimit(0.0f, 1.0f, juce::jmap(mAmt, pAirAmt->load(), targetB.airAmt) + macroY * 0.22f);
    const float airAlpha = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 5500.0f / static_cast<float>(currentSampleRate));
    const float dcCoeff = 1.0f - (juce::MathConstants<float>::twoPi * 20.0f / static_cast<float>(currentSampleRate));
    smoothedMaster.setTargetValue(pMasterGain->load() * 0.65f);
    float blockPeak = 0.0f;
    int sPos = scopeWritePos.load(std::memory_order_relaxed);
    const float tapeGain = tapeStopMult < 0.08f ? (tapeStopMult / 0.08f) : 1.0f;

    for (int i = 0; i < nSamples; ++i)
    {
        if (air > 0.01f)
        {
            airLpL += airAlpha * (L[i] - airLpL);
            airLpR += airAlpha * (R[i] - airLpR);
            L[i] += std::tanh((L[i] - airLpL) * 2.4f) * air * 0.42f;
            R[i] += std::tanh((R[i] - airLpR) * 2.4f) * air * 0.42f;
        }

        const float dcL = L[i] - dcInL + dcCoeff * dcOutL; dcInL = L[i]; dcOutL = dcL;
        const float dcR = R[i] - dcInR + dcCoeff * dcOutR; dcInR = R[i]; dcOutR = dcR;

        const float m = smoothedMaster.getNextValue() * tapeGain;
        float outSampleL = std::tanh(dcL * m);
        float outSampleR = std::tanh(dcR * m);
        if (!std::isfinite(outSampleL)) outSampleL = 0.0f;
        if (!std::isfinite(outSampleR)) outSampleR = 0.0f;
        L[i] = outSampleL;
        R[i] = outSampleR;

        blockPeak = juce::jmax(blockPeak, std::abs(outSampleL), std::abs(outSampleR));
        scopeBuffer[static_cast<size_t>(sPos)] = 0.5f * (outSampleL + outSampleR);
        sPos = (sPos + 1) % scopeSize;
    }

    scopeWritePos.store(sPos, std::memory_order_relaxed);
    peakLevel.store(blockPeak, std::memory_order_relaxed);
}

void WitchSynthAudioProcessor::renderAudioSegment(juce::AudioBuffer<float>& buffer, int startSample, int numSamples,
                                                  double bpm, bool hostPlaying, double hostPpq)
{
    const auto& allPresets = getAllPresets();
    const int morphIdx = juce::jlimit(0, static_cast<int>(allPresets.size()) - 1, static_cast<int>(pMorphPreset->load()));
    const auto& targetB = allPresets[static_cast<size_t>(morphIdx)];
    smoothedMorphAmt.setTargetValue(juce::jlimit(0.0f, 1.0f, pMorphAmt->load()));
    const float mAmt = smoothedMorphAmt.getNextValue();
    smoothedMorphAmt.skip(juce::jmax(0, numSamples - 1));

    const float macroX = (pMacroX->load() - 0.5f) * 2.0f;

    const int osc1Wave = static_cast<int>(pOsc1Wave->load());
    const int osc1Oct = static_cast<int>(pOsc1Oct->load());
    const int osc1Semi = static_cast<int>(pOsc1Semi->load());
    const float detune = juce::jmap(mAmt, pOsc1Detune->load(), targetB.osc1Detune);
    const float osc1Lvl = pOsc1Level->load();
    const float basePw = juce::jlimit(0.08f, 0.92f, juce::jmap(mAmt, pPwmWidth->load(), targetB.pwmWidth) + macroX * 0.16f);
    const float xMod = juce::jlimit(0.0f, 1.0f, juce::jmap(mAmt, pXModAmt->load(), targetB.xModAmt) + macroX * 0.22f);

    const int osc2Wave = static_cast<int>(pOsc2Wave->load());
    const int osc2Oct = static_cast<int>(pOsc2Oct->load());
    const int osc2Semi = static_cast<int>(pOsc2Semi->load());
    const int unison = juce::jlimit(1, Voice::maxUnison, static_cast<int>(pOsc2Unison->load()));
    const float osc2Lvl = pOsc2Level->load();

    const float subLvl = juce::jmap(mAmt, pSubLevel->load(), targetB.subLevel);
    const float entropy = juce::jmap(mAmt, pEntropyAmt->load(), targetB.entropyAmt);
    const float gravity = juce::jmap(mAmt, pGravityAmt->load(), targetB.gravityAmt);
    const float ghostVox = juce::jmap(mAmt, pGhostVoxAmt->load(), targetB.ghostVoxAmt);
    const float noiseLvl = juce::jmap(mAmt, pNoiseLevel->load(), targetB.noiseLevel);
    const float vhsDrift = juce::jmap(mAmt, pVhsDrift->load(), targetB.vhsDrift);

    const int fType = static_cast<int>(pFilterType->load());
    const float baseCut = pFilterCutoff->load();
    const float morphedCut = baseCut * std::pow(juce::jmax(20.0f, targetB.filterCutoff) / juce::jmax(20.0f, baseCut), mAmt);
    smoothedCutoff.setTargetValue(morphedCut * std::pow(2.0f, macroX * 1.8f));
    const float fReso = juce::jlimit(0.5f, 12.0f, juce::jmap(mAmt, pFilterReso->load(), targetB.filterReso));
    const float fDrive = 1.0f + juce::jmap(mAmt, pFilterDrive->load(), targetB.filterDrive) * 5.0f;
    const float fEnvAmt = juce::jmap(mAmt, pFilterEnvAmt->load(), targetB.filterEnvAmt);
    const float fKeyTrack = pFilterKeyTrack->load();
    const float pPunch = pPitchPunch->load();
    const float snapAmt = pSnapAmt->load();

    const juce::ADSR::Parameters ampParams{ pAmpA->load(), pAmpD->load(), pAmpS->load(), pAmpR->load() };
    const juce::ADSR::Parameters fEnvParams{ pAmpA->load() * 0.35f + 0.001f, pFilterDecay->load(), pAmpS->load() > 0.5f ? 0.2f : 0.0f, pAmpR->load() };

    const float lfoRate = juce::jmap(mAmt, pLfoRate->load(), targetB.lfoRate);
    const float lfoDepth = juce::jmap(mAmt, pLfoDepth->load(), targetB.lfoDepth);
    const int lfoTarget = static_cast<int>(pLfoTarget->load());
    const int lfoShape = static_cast<int>(pLfoShape->load());

    const int arpMode = static_cast<int>(pArpMode->load());
    const int arpRateIdx = juce::jlimit(0, 5, static_cast<int>(pArpRate->load()));
    static constexpr double arpDiv[6] = { 1.0, 2.0, 3.0, 4.0, 6.0, 8.0 };
    const double stepInc = (bpm / 60.0) * arpDiv[arpRateIdx] / currentSampleRate;

    int seqVals[8];
    for (int s = 0; s < 8; ++s)
        seqVals[s] = static_cast<int>(pSeq[static_cast<size_t>(s)]->load());

    const float srF = static_cast<float>(currentSampleRate);
    const float invSr = 1.0f / srF;
    const float srScale = 44100.0f / srF;
    const bool tapeStopOn = pTapeStop->load() > 0.5f;
    const float glideTime = pGlideTime->load();
    const float glideCoeff = glideTime > 0.001f ? 1.0f - std::exp(-1.0f / (glideTime * srF)) : 1.0f;
    const float pitchEnvCoeff = std::exp(-1.0f / (0.032f * srF));
    const float snapEnvCoeff = std::exp(-1.0f / (0.014f * srF));
    const float stealInc = 1.0f / (0.0015f * srF);
    const float combHpAlpha = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 60.0f / srF);

    const float osc1Ratio = std::pow(2.0f, static_cast<float>(osc1Oct) + static_cast<float>(osc1Semi) / 12.0f);
    const float osc2Ratio = std::pow(2.0f, static_cast<float>(osc2Oct) + static_cast<float>(osc2Semi) / 12.0f);
    constexpr float subRatio = 0.5f;
    const float unisonNorm = 1.0f / std::sqrt(static_cast<float>(unison));

    std::array<float, Voice::maxUnison> detMults{}, detInvMults{}, panLs{}, panRs{};
    for (int u = 0; u < unison; ++u)
    {
        const float normPos = unison > 1 ? (2.0f * static_cast<float>(u) / static_cast<float>(unison - 1) - 1.0f) : 0.0f;
        const float curvePos = 0.55f * normPos + 0.45f * normPos * std::abs(normPos);
        detMults[static_cast<size_t>(u)] = std::pow(2.0f, curvePos * detune * 0.048f);
        detInvMults[static_cast<size_t>(u)] = 1.0f / detMults[static_cast<size_t>(u)];
        panLs[static_cast<size_t>(u)] = 0.5f * (1.0f - normPos * 0.80f);
        panRs[static_cast<size_t>(u)] = 0.5f * (1.0f + normPos * 0.80f);
    }

    for (auto& v : voices)
    {
        v.ampEnv.setParameters(ampParams);
        v.filterEnv.setParameters(fEnvParams);
        if (fType == 3)
        {
            v.filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            v.formant2.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            v.filter.setResonance(juce::jlimit(1.2f, 14.0f, fReso * 1.3f));
            v.formant2.setResonance(juce::jlimit(1.2f, 14.0f, fReso * 1.5f));
        }
        else if (fType == 5)
        {
            v.filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            v.formant2.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            v.filter.setResonance(juce::jlimit(0.5f, 10.0f, fReso * 0.85f));
            v.formant2.setResonance(juce::jlimit(0.5f, 10.0f, fReso * 0.85f));
        }
        else
        {
            v.filter.setType(fType == 0 ? juce::dsp::StateVariableTPTFilterType::lowpass
                           : fType == 1 ? juce::dsp::StateVariableTPTFilterType::highpass
                                        : juce::dsp::StateVariableTPTFilterType::bandpass);
            v.filter.setResonance(juce::jlimit(0.5f, 10.5f, fReso));
        }
    }

    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getWritePointer(1);
    const float maxCutoff = juce::jmin(19500.0f, srF * 0.43f);

    static constexpr float f1Table[6] = { 730.0f, 530.0f, 270.0f, 570.0f, 300.0f, 650.0f };
    static constexpr float f2Table[6] = { 1090.0f, 1840.0f, 2290.0f, 840.0f, 870.0f, 2400.0f };
    static constexpr int chipOctPattern[8]  = { 0, 12, 0, 12, 7, 12, 0, 24 };
    static constexpr int chaosOctPattern[8] = { 0, 12, -12, 7, 15, 0, 19, 12 };

    double curArpPos = hostPlaying ? (std::max(0.0, hostPpq) * arpDiv[arpRateIdx]) : arpPhase;

    for (int i = 0; i < numSamples; ++i)
    {
        if (tapeStopOn)
            tapeStopMult += (0.015f - tapeStopMult) * (0.00022f * srScale);
        else
            tapeStopMult += (1.0f - tapeStopMult) * (0.0014f * srScale);

        const int stepIdx = static_cast<int>(std::floor(curArpPos));
        const double stepFrac = curArpPos - static_cast<double>(stepIdx);
        const int stType = seqVals[stepIdx & 7];

        if (arpMode == 1)
        {
            float targetGate = 1.0f;
            if (stType == 0) targetGate = 0.0f;
            else if (stType == 3) targetGate = (stepFrac < 0.36 || (stepFrac >= 0.5 && stepFrac < 0.86)) ? 1.0f : 0.0f;
            else targetGate = stepFrac < 0.58 ? 1.0f : 0.0f;
            arpGateSmooth += 0.09f * (targetGate - arpGateSmooth);
            arpStepDisplay.store(stepIdx & 7, std::memory_order_relaxed);
        }
        else if (arpMode >= 2 && !heldNotes.empty())
        {
            arpGateSmooth = 1.0f;
            auto triggerStepNote = [&](int accentAdd)
            {
                const int nHeld = static_cast<int>(heldNotes.size());
                const int baseNote = heldNotes[static_cast<size_t>(stepIdx % nHeld)];
                const int patOffset = (nHeld == 1 || arpMode == 3)
                                    ? (arpMode == 2 ? chipOctPattern[stepIdx & 7] : chaosOctPattern[stepIdx & 7])
                                    : ((stepIdx / nHeld) % 2 == 1 ? 12 : 0);
                const int note = juce::jlimit(12, 115, baseNote + patOffset + accentAdd);
                auto& v0 = voices[0];
                v0.active = true;
                v0.midiNote = note;
                v0.velocity = accentAdd > 0 ? 1.0f : 0.85f;
                v0.targetFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(note));
                if (glideTime <= 0.005f) v0.currentFreq = v0.targetFreq;
                v0.pitchEnvVal = 1.0f;
                v0.snapEnvVal = 1.0f;
                v0.ampEnv.noteOn();
                v0.filterEnv.noteOn();
                arpGateOpen = true;
            };

            if (stepIdx != lastArpStep)
            {
                lastArpStep = stepIdx;
                lastSubHit = 0;
                arpStepDisplay.store(stepIdx & 7, std::memory_order_relaxed);
                if (stType > 0)
                    triggerStepNote(stType == 2 ? 12 : 0);
            }
            if (stType == 3 && stepFrac >= 0.5 && lastSubHit == 0)
            {
                lastSubHit = 1;
                triggerStepNote(0);
            }
            const bool shouldClose = (stType == 3) ? ((stepFrac >= 0.38 && stepFrac < 0.5) || stepFrac >= 0.88) : (stepFrac >= 0.65);
            if (arpGateOpen && shouldClose)
            {
                voices[0].ampEnv.noteOff();
                voices[0].filterEnv.noteOff();
                arpGateOpen = false;
            }
        }
        else
        {
            arpGateSmooth = 1.0f;
        }
        curArpPos += stepInc;

        lfoPhase += lfoRate * invSr;
        if (lfoPhase >= 1.0f) { lfoPhase -= 1.0f; lfoSampleHold = nextNoise(); }

        float lfoVal = 0.0f;
        if (lfoShape == 0)      lfoVal = fastSin(lfoPhase);
        else if (lfoShape == 1) lfoVal = lfoPhase < 0.5f ? 1.0f : -1.0f;
        else if (lfoShape == 2) lfoVal = 1.0f - 2.0f * lfoPhase;
        else if (lfoShape == 3) lfoVal = lfoSampleHold;
        else
        {
            const double lDt = juce::jlimit(1e-6, 0.004, static_cast<double>(lfoRate) * 0.65 * static_cast<double>(invSr));
            const double dx = 10.0 * (lorenzY - lorenzX);
            const double dy = lorenzX * (28.0 - lorenzZ) - lorenzY;
            const double dz = lorenzX * lorenzY - (8.0 / 3.0) * lorenzZ;
            lorenzX = juce::jlimit(-30.0, 30.0, lorenzX + dx * lDt);
            lorenzY = juce::jlimit(-30.0, 30.0, lorenzY + dy * lDt);
            lorenzZ = juce::jlimit(0.0, 60.0, lorenzZ + dz * lDt);
            if (!std::isfinite(lorenzX) || !std::isfinite(lorenzY) || !std::isfinite(lorenzZ))
            {
                lorenzX = 0.1; lorenzY = 0.0; lorenzZ = 0.0;
            }
            lfoVal = juce::jlimit(-1.0f, 1.0f, static_cast<float>(lorenzX / 18.0));
        }

        vhsWowPhase += 0.55f * invSr;
        if (vhsWowPhase >= 1.0f) vhsWowPhase -= 1.0f;
        const float rawNoise = nextNoise();
        vhsSmoothNoise += 0.0008f * (rawNoise - vhsSmoothNoise);
        const float vhsMod = vhsDrift * (0.012f * fastSin(vhsWowPhase) + 0.022f * vhsSmoothNoise);

        const float gravBend = 1.0f - gravity * juce::jlimit(0.0f, 0.14f, gravityEnergy * 0.12f);
        float pitchMult = pitchBendRatio * tapeStopMult * gravBend * (1.0f + vhsMod);
        if (lfoTarget == 0)
            pitchMult *= std::pow(2.0f, lfoVal * lfoDepth * 0.08f);

        const float pwMod = juce::jlimit(0.06f, 0.94f, basePw + (lfoTarget == 2 ? lfoVal * lfoDepth * 0.36f : 0.0f));
        const float curCutoff = smoothedCutoff.getNextValue();
        float sumL = 0.0f, sumR = 0.0f;
        const float noiseSample = rawNoise * noiseLvl * 0.25f;

        for (auto& v : voices)
        {
            if (!v.active) continue;

            v.currentFreq += (v.targetFreq - v.currentFreq) * glideCoeff;
            v.pitchEnvVal *= pitchEnvCoeff;
            v.snapEnvVal *= snapEnvCoeff;
            if (v.stealGain < 1.0f) v.stealGain = juce::jmin(1.0f, v.stealGain + stealInc);

            const float envVal = v.ampEnv.getNextSample();
            const float fEnvVal = v.filterEnv.getNextSample();

            if (!v.ampEnv.isActive())
            {
                v.active = false;
                v.keyDown = false;
                continue;
            }

            const float punchMult = 1.0f + pPunch * v.pitchEnvVal * 2.5f;
            const float voicePitch = pitchMult * punchMult * (1.0f + v.entropyPitch * entropy);
            float vL = 0.0f, vR = 0.0f;
            const float baseFreq1 = v.currentFreq * osc1Ratio * voicePitch;
            const float baseFreq2 = v.currentFreq * osc2Ratio * voicePitch;
            const float gravOrbit = gravity * 0.10f * fastSin(v.subPhase * 2.0f);

            for (int u = 0; u < unison; ++u)
            {
                const float dt2 = juce::jlimit(1e-5f, 0.45f, (baseFreq2 * detInvMults[static_cast<size_t>(u)]) * invSr);
                v.phase2[static_cast<size_t>(u)] += dt2;
                if (v.phase2[static_cast<size_t>(u)] >= 1.0f) v.phase2[static_cast<size_t>(u)] -= 1.0f;
                const float s2Raw = generateOsc(osc2Wave, v.phase2[static_cast<size_t>(u)], dt2, pwMod);

                const float dt1 = juce::jlimit(1e-5f, 0.45f, (baseFreq1 * detMults[static_cast<size_t>(u)]) * invSr);
                v.phase1[static_cast<size_t>(u)] += dt1;
                if (v.phase1[static_cast<size_t>(u)] >= 1.0f) v.phase1[static_cast<size_t>(u)] -= 1.0f;
                float modPhase1 = v.phase1[static_cast<size_t>(u)] + xMod * s2Raw * 0.25f + gravOrbit;
                modPhase1 = modPhase1 - std::floor(modPhase1);
                const float s1 = generateOsc(osc1Wave, modPhase1, dt1, pwMod) * osc1Lvl;

                const float monoS = (s1 + s2Raw * osc2Lvl) * unisonNorm;
                vL += monoS * panLs[static_cast<size_t>(u)];
                vR += monoS * panRs[static_cast<size_t>(u)];
            }

            const float subDt = juce::jlimit(1e-5f, 0.45f, (v.currentFreq * subRatio * voicePitch) * invSr);
            v.subPhase += subDt;
            if (v.subPhase >= 1.0f) v.subPhase -= 1.0f;
            const float subSine = fastSin(v.subPhase);
            const float subSqr = v.subPhase < 0.5f ? 0.7f : -0.7f;
            const float subDirty = subLvl > 0.5f ? (subLvl - 0.5f) * 2.0f : 0.0f;
            const float subS = (subSine * (1.0f - subDirty * 0.5f) + subSqr * subDirty * 0.5f) * subLvl * 0.65f;

            float ghostL = 0.0f, ghostR = 0.0f;
            if (ghostVox > 0.01f)
            {
                const float gDt1 = juce::jlimit(1e-5f, 0.45f, (v.currentFreq * 1.4983f * voicePitch) * invSr);
                const float gDt2 = juce::jlimit(1e-5f, 0.45f, (v.currentFreq * 2.0f * voicePitch) * invSr);
                v.ghostPhase1 += gDt1; if (v.ghostPhase1 >= 1.0f) v.ghostPhase1 -= 1.0f;
                v.ghostPhase2 += gDt2; if (v.ghostPhase2 >= 1.0f) v.ghostPhase2 -= 1.0f;
                const float gTail = (1.0f - 0.55f * v.snapEnvVal) * ghostVox * 0.40f;
                ghostL = (fastSin(v.ghostPhase1) * 0.65f + fastSin(v.ghostPhase2) * 0.55f) * gTail;
                ghostR = (fastSin(v.ghostPhase2) * 0.65f + fastSin(v.ghostPhase1) * 0.55f) * gTail;
            }

            vL = std::tanh((vL + subS + ghostL + noiseSample) * fDrive);
            vR = std::tanh((vR + subS + ghostR + noiseSample) * fDrive);

            const float keyTrackRatio = std::pow(v.currentFreq / 261.63f, fKeyTrack);
            float cutoff = curCutoff * keyTrackRatio * std::pow(2.0f, fEnvVal * fEnvAmt * 4.5f + v.entropyCutoff * entropy);
            if (lfoTarget == 1)
                cutoff *= std::pow(2.0f, lfoVal * lfoDepth * 2.5f);
            // Improvement 1: velocity -> brightness (на полной велосити = 1.0, без изменений)
            cutoff *= (1.0f - 0.22f * (1.0f - v.velocity));
            cutoff = juce::jlimit(20.0f, maxCutoff, cutoff);

            if (fType == 3)
            {
                const float vowelPos = juce::jlimit(0.0f, 4.999f, (std::log2(juce::jmax(150.0f, cutoff) / 150.0f) / 6.5f) * 5.0f);
                const int idx0 = static_cast<int>(vowelPos);
                const float frac = vowelPos - static_cast<float>(idx0);
                const float f1 = f1Table[idx0] * (1.0f - frac) + f1Table[idx0 + 1] * frac;
                const float f2 = f2Table[idx0] * (1.0f - frac) + f2Table[idx0 + 1] * frac;
                v.filter.setCutoffFrequency(juce::jlimit(20.0f, maxCutoff, f1));
                v.formant2.setCutoffFrequency(juce::jlimit(20.0f, maxCutoff, f2));
                vL = v.filter.processSample(0, vL) * 0.85f + v.formant2.processSample(0, vL) * 0.70f;
                vR = v.filter.processSample(1, vR) * 0.85f + v.formant2.processSample(1, vR) * 0.70f;
            }
            else if (fType == 4)
            {
                const float noteFreq = v.currentFreq * voicePitch;
                const float cFreq = fKeyTrack >= 0.85f ? noteFreq
                                  : juce::jmap(fKeyTrack / 0.85f, juce::jlimit(30.0f, 3500.0f, cutoff), noteFreq);
                const float dSamps = juce::jlimit(2.0f, static_cast<float>(Voice::combSize - 2), srF / juce::jmax(25.0f, cFreq));
                const int dInt = static_cast<int>(dSamps);
                const float frac = dSamps - static_cast<float>(dInt);
                const int r0 = (v.combPos - dInt + Voice::combSize) % Voice::combSize;
                const int r1 = (v.combPos - dInt - 1 + Voice::combSize) % Voice::combSize;
                const float cL = v.combBufL[static_cast<size_t>(r0)] * (1.0f - frac) + v.combBufL[static_cast<size_t>(r1)] * frac;
                const float cR = v.combBufR[static_cast<size_t>(r0)] * (1.0f - frac) + v.combBufR[static_cast<size_t>(r1)] * frac;

                const float damp = juce::jlimit(0.08f, 0.90f, cutoff / 4500.0f);
                v.combLpL += damp * (cL - v.combLpL);
                v.combLpR += damp * (cR - v.combLpR);
                v.combHpL += combHpAlpha * (v.combLpL - v.combHpL);
                v.combHpR += combHpAlpha * (v.combLpR - v.combHpR);
                const float cleanL = v.combLpL - v.combHpL;
                const float cleanR = v.combLpR - v.combHpR;
                const float fb = juce::jlimit(0.0f, 0.92f, ((fReso - 0.5f) / 11.5f) * 0.92f);
                v.combBufL[static_cast<size_t>(v.combPos)] = std::tanh(vL * 0.50f + cleanL * fb);
                v.combBufR[static_cast<size_t>(v.combPos)] = std::tanh(vR * 0.50f + cleanR * fb);
                v.combPos = (v.combPos + 1) % Voice::combSize;
                vL = vL * 0.50f + cleanL * 0.72f;
                vR = vR * 0.50f + cleanR * 0.72f;
            }
            else if (fType == 5)
            {
                v.filter.setCutoffFrequency(cutoff);
                v.formant2.setCutoffFrequency(cutoff);
                vL = v.formant2.processSample(0, std::tanh(v.filter.processSample(0, vL) * 1.45f));
                vR = v.formant2.processSample(1, std::tanh(v.filter.processSample(1, vR) * 1.45f));
            }
            else
            {
                v.filter.setCutoffFrequency(cutoff);
                vL = v.filter.processSample(0, vL);
                vR = v.filter.processSample(1, vR);
            }

            const float snapBoost = 1.0f + snapAmt * v.snapEnvVal * 1.25f;
            // Improvement 2: музыкальная велосити-кривая (на велосити=1.0 звучит как раньше)
            const float velCurve = v.velocity * v.velocity * 0.55f + v.velocity * 0.45f;
            const float gain = envVal * velCurve * arpGateSmooth * v.stealGain * snapBoost * 0.42f;
            sumL += vL * gain;
            sumR += vR * gain;
        }

        const float monoE = 0.5f * (std::abs(sumL) + std::abs(sumR));
        gravityEnergy += 0.004f * (monoE - gravityEnergy);

        outL[startSample + i] += sumL;
        outR[startSample + i] += sumR;
    }

    arpPhase = curArpPos;
}

void WitchSynthAudioProcessor::setCurrentProgram(int index) { loadPreset(index); }

const juce::String WitchSynthAudioProcessor::getProgramName(int index)
{
    const auto& presets = getAllPresets();
    if (index >= 0 && index < static_cast<int>(presets.size()))
        return presets[static_cast<size_t>(index)].name;
    return {};
}

void WitchSynthAudioProcessor::initPatch()
{
    auto setP = [this](const char* id, float val)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(val));
    };

    setP("osc1Wave", 0.0f); setP("osc1Oct", 0.0f); setP("osc1Semi", 0.0f);
    setP("osc1Detune", 0.25f); setP("osc1Level", 0.85f); setP("pwmWidth", 0.50f); setP("xModAmt", 0.0f);
    setP("osc2Wave", 0.0f); setP("osc2Oct", -1.0f); setP("osc2Semi", 0.0f);
    setP("osc2Unison", 3.0f); setP("osc2Level", 0.55f);
    setP("subLevel", 0.35f); setP("entropyAmt", 0.10f); setP("gravityAmt", 0.0f); setP("ghostVoxAmt", 0.0f);
    setP("noiseLevel", 0.0f); setP("vhsDrift", 0.05f); setP("crushAmount", 0.0f); setP("tiltAmt", 0.0f); setP("lowCutHz", 22.0f);
    setP("filterType", 0.0f); setP("filterCutoff", 12000.0f); setP("filterReso", 1.2f);
    setP("filterDrive", 0.15f); setP("filterEnvAmt", 0.15f); setP("filterDecay", 0.35f); setP("filterKeyTrack", 0.6f); setP("pitchPunch", 0.0f);
    setP("ampAttack", 0.005f); setP("ampDecay", 0.35f); setP("ampSustain", 0.85f); setP("ampRelease", 0.25f);
    setP("snapAmt", 0.0f); setP("airAmt", 0.20f);
    setP("lfoRate", 1.5f); setP("lfoDepth", 0.0f); setP("lfoTarget", 0.0f); setP("lfoShape", 0.0f);
    setP("polyMode", 0.0f); setP("arpMode", 0.0f); setP("arpRate", 3.0f);
    setP("glideTime", 0.02f); setP("stereoWidth", 0.55f); setP("pumpAmt", 0.0f);
    for (int s = 0; s < 8; ++s) setP(("seq" + juce::String(s)).toRawUTF8(), 1.0f);
    setP("distOn", 0.0f); setP("distMode", 0.0f); setP("distDrive", 0.25f); setP("distMix", 0.85f);
    setP("chorusOn", 0.0f); setP("chorusMix", 0.30f); setP("chorusRate", 1.1f); setP("chorusDepth", 0.38f);
    setP("delayOn", 0.0f); setP("delayMode", 1.0f); setP("delayTime", 0.25f); setP("delayFeedback", 0.35f); setP("delayMix", 0.25f);
    setP("reverbOn", 0.0f); setP("reverbFreeze", 0.0f); setP("reverbSize", 0.55f); setP("reverbMix", 0.25f); setP("haloAmt", 0.0f);
    setP("morphAmt", 0.0f); setP("tapeStop", 0.0f); setP("macroX", 0.5f); setP("macroY", 0.5f); setP("masterGain", 0.78f);
    resetFxFlag.store(true);
}

void WitchSynthAudioProcessor::loadPreset(int index)
{
    const auto& presets = getAllPresets();
    if (presets.empty()) return;
    currentPresetIndex = juce::jlimit(0, static_cast<int>(presets.size()) - 1, index);
    const auto& p = presets[static_cast<size_t>(currentPresetIndex)];
    const bool keepFx = lockFx.load();

    auto setP = [this](const char* id, float val)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(val));
    };

    setP("osc1Wave", static_cast<float>(p.osc1Wave));
    setP("osc1Oct", static_cast<float>(p.osc1Oct));
    setP("osc1Semi", static_cast<float>(p.osc1Semi));
    setP("osc1Detune", p.osc1Detune);
    setP("osc1Level", p.osc1Level);
    setP("pwmWidth", p.pwmWidth);
    setP("xModAmt", p.xModAmt);
    setP("osc2Wave", static_cast<float>(p.osc2Wave));
    setP("osc2Oct", static_cast<float>(p.osc2Oct));
    setP("osc2Semi", static_cast<float>(p.osc2Semi));
    setP("osc2Unison", static_cast<float>(p.osc2Unison));
    setP("osc2Level", p.osc2Level);
    setP("subLevel", p.subLevel);
    setP("entropyAmt", p.entropyAmt);
    setP("gravityAmt", p.gravityAmt);
    setP("ghostVoxAmt", p.ghostVoxAmt);
    setP("noiseLevel", p.noiseLevel);
    setP("vhsDrift", p.vhsDrift);
    setP("crushAmount", p.crushAmount);
    setP("tiltAmt", p.tiltAmt);
    setP("lowCutHz", (p.folderIndex == 2 || p.folderIndex == 5) ? 95.0f : 24.0f);
    setP("filterType", static_cast<float>(p.filterType));
    setP("filterCutoff", p.filterCutoff);
    setP("filterReso", p.filterReso);
    setP("filterDrive", p.filterDrive);
    setP("filterEnvAmt", p.filterEnvAmt);
    setP("filterDecay", p.filterDecay);
    setP("filterKeyTrack", p.filterKeyTrack);
    setP("pitchPunch", p.pitchPunch);
    setP("ampAttack", p.ampA);
    setP("ampDecay", p.ampD);
    setP("ampSustain", p.ampS);
    setP("ampRelease", p.ampR);
    setP("snapAmt", p.snapAmt);
    setP("airAmt", p.airAmt);
    setP("lfoRate", p.lfoRate);
    setP("lfoDepth", p.lfoDepth);
    setP("lfoTarget", static_cast<float>(p.lfoTarget));
    setP("lfoShape", static_cast<float>(p.lfoShape));
    setP("polyMode", static_cast<float>(p.polyMode));
    setP("arpMode", static_cast<float>(p.arpMode));
    setP("arpRate", static_cast<float>(p.arpRate));
    setP("glideTime", p.glideTime);
    setP("stereoWidth", p.stereoWidth);
    setP("pumpAmt", p.pumpAmt);
    for (int s = 0; s < 8; ++s)
        setP(("seq" + juce::String(s)).toRawUTF8(), static_cast<float>(p.seq[static_cast<size_t>(s)]));

    setP("distOn", p.distOn ? 1.0f : 0.0f);
    setP("distMode", static_cast<float>(p.distMode));
    setP("distDrive", p.distDrive);
    setP("distMix", 0.85f);
    setP("chorusOn", p.chorusOn ? 1.0f : 0.0f);
    setP("chorusMix", p.chorusMix);
    setP("chorusRate", 1.1f);
    setP("chorusDepth", 0.38f);

    if (!keepFx)
    {
        setP("delayOn", p.delayOn ? 1.0f : 0.0f);
        setP("delayMode", static_cast<float>(p.delayMode));
        setP("delayTime", p.delayTime);
        setP("delayFeedback", p.delayFb);
        setP("delayMix", p.delayMix);
        setP("reverbOn", p.reverbOn ? 1.0f : 0.0f);
        setP("reverbFreeze", p.reverbFreeze ? 1.0f : 0.0f);
        setP("reverbSize", p.reverbSize);
        setP("reverbMix", p.reverbMix);
        setP("haloAmt", p.haloAmt);
        setP("morphAmt", 0.0f);
        setP("tapeStop", 0.0f);
        setP("macroX", 0.5f);
        setP("macroY", 0.5f);
        resetFxFlag.store(true);
    }
    setP("masterGain", p.masterGain);
}

juce::File WitchSynthAudioProcessor::getFavoritesFile() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("WitchAudio")
        .getChildFile("WitchSynth_Favorites.xml");
}

juce::File WitchSynthAudioProcessor::getUserPresetsDir() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("WitchAudio")
        .getChildFile("UserPresets");
    dir.createDirectory();
    return dir;
}

void WitchSynthAudioProcessor::saveUserPreset(const juce::String& name)
{
    auto safe = name.trim();
    if (safe.isEmpty()) safe = "Untitled";
    safe = safe.retainCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 _-");
    if (safe.isEmpty()) safe = "Untitled";

    auto file = getUserPresetsDir().getChildFile(safe + ".witch");
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        xml->writeTo(file);
}

juce::StringArray WitchSynthAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    auto dir = const_cast<WitchSynthAudioProcessor*>(this)->getUserPresetsDir();
    for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.witch"))
        names.add(f.getFileNameWithoutExtension());
    names.sort(true);
    return names;
}

bool WitchSynthAudioProcessor::loadUserPreset(const juce::String& name)
{
    auto file = getUserPresetsDir().getChildFile(name + ".witch");
    if (!file.existsAsFile()) return false;
    if (auto xml = juce::XmlDocument::parse(file))
    {
        if (xml->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
            resetFxFlag.store(true);
            return true;
        }
    }
    return false;
}

bool WitchSynthAudioProcessor::isFavorite(int presetIdx) const
{
    return favoritePresets.count(presetIdx) > 0;
}

void WitchSynthAudioProcessor::toggleFavorite(int presetIdx)
{
    if (presetIdx < 0 || presetIdx >= getNumPrograms()) return;
    if (favoritePresets.count(presetIdx) > 0) favoritePresets.erase(presetIdx);
    else favoritePresets.insert(presetIdx);
    saveFavorites();
}

std::vector<int> WitchSynthAudioProcessor::getFavoriteIndices() const
{
    return { favoritePresets.begin(), favoritePresets.end() };
}

void WitchSynthAudioProcessor::saveFavorites()
{
    auto file = getFavoritesFile();
    file.getParentDirectory().createDirectory();
    juce::XmlElement root("WitchSynthFavorites");
    for (int idx : favoritePresets)
    {
        auto* item = root.createNewChildElement("Preset");
        item->setAttribute("index", idx);
    }
    root.writeTo(file);
}

void WitchSynthAudioProcessor::loadFavorites()
{
    favoritePresets.clear();
    auto file = getFavoritesFile();
    if (!file.existsAsFile()) return;
    if (auto xml = juce::XmlDocument::parse(file))
    {
        const int total = getNumPrograms();
        for (auto* child : xml->getChildIterator())
        {
            if (child->hasTagName("Preset"))
            {
                const int idx = child->getIntAttribute("index", -1);
                if (idx >= 0 && idx < total)
                    favoritePresets.insert(idx);
            }
        }
    }
}

void WitchSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("presetIndex", currentPresetIndex, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void WitchSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        const int presetCount = static_cast<int>(getAllPresets().size());
        const int savedPresetIndex = static_cast<int>(tree.getProperty("presetIndex", 0));
        currentPresetIndex = presetCount > 0 ? juce::jlimit(0, presetCount - 1, savedPresetIndex) : 0;
        apvts.replaceState(tree);
        resetFxFlag.store(true);
    }
}

juce::AudioProcessorEditor* WitchSynthAudioProcessor::createEditor()
{
    return new WitchSynthAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WitchSynthAudioProcessor();
}
