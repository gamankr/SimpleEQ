/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

struct ChainSettings
{
    float peakFreq{0}, peakGainInDecibels{0}, peakQuality{1.f};
    float lowCutFreq{0}, highCutFreq{0};
    Slope lowCutSlope{Slope::Slope_12}, highCutSlope{Slope::Slope_12};
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts); 

//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    juce::AudioProcessorValueTreeState apvts = juce::AudioProcessorValueTreeState(*this, nullptr, "Parameters", createParameterLayout());

private:

    // Type aliasing - Only Float filters for this project
    using Filter = juce::dsp::IIR::Filter<float>;

    /* Each Filter processes 12 dB/Oct if configured as LowCut ( also called HighPass) /
     HighCut (LowPass) Filter. Since the LowCut Slope/ HighCut Slope can go upto 48 dB/Oct,
     we need 4 of these filters in a ProcessorChain*/
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

    /* Create a ProcessorChain with required Filters. Here,
       LowCut -> Parameteric -> HighCut */
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

    // Two MonoChain objects required for the Stereo Processing
    MonoChain leftChain, rightChain;
    
    // Represents each link's position in the chain
    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut
    };
    
    void updatePeakFilter(const ChainSettings& chainSettings);
    
    using Coefficients = Filter::CoefficientsPtr;
    static void updateCoefficients(Coefficients& old, const Coefficients& replacements);
    
    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& selectedLowCutFilter,
                        const CoefficientType& cutCoefficients,
                        const Slope& lowCutSlope)
    {
        
        
        // Bypass links in the chain
        selectedLowCutFilter.template setBypassed<0>(true);
        selectedLowCutFilter.template setBypassed<1>(true);
        selectedLowCutFilter.template setBypassed<2>(true);
        selectedLowCutFilter.template setBypassed<3>(true);
        
        switch(lowCutSlope)
        {
                /*
                 switch statement code refactored to below code using switch case pass-through trick.
                 Check link: https://stackoverflow.com/questions/8146106/does-case-switch-work-like-this
                 Remove "break" statement for pass-through.
                 Check commit for previous code: "Refactoring using switch case pass-through trick"
                 */
                    
                case Slope_48:
                {
                    *selectedLowCutFilter.template get<3>().coefficients = *cutCoefficients[3];
                    selectedLowCutFilter.template setBypassed<3>(false);
                }
                case Slope_36:
                {
                    *selectedLowCutFilter.template get<2>().coefficients = *cutCoefficients[2];
                    selectedLowCutFilter.template setBypassed<2>(false);

                }
                case Slope_24:
                {
                    *selectedLowCutFilter.template get<1>().coefficients = *cutCoefficients[1];
                    selectedLowCutFilter.template setBypassed<1>(false);
                }
                case Slope_12:
                {
                    *selectedLowCutFilter.template get<0>().coefficients = *cutCoefficients[0];
                    selectedLowCutFilter.template setBypassed<0>(false);
                }
        }
        
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};


