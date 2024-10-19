// src/audio/WebAudioSource.cpp
#include "IAudioSource.h"
#include <vector>
#include <mutex>
#include <emscripten.h>
#include <memory>

class WebAudioSource : public IAudioSource {
        std::string mDescription;

public:
    WebAudioSource() : m_samples(), m_sampleRate(44100) {
        // Initialize the WebAssembly audio worklet
        InitAudioWorklet();

        mDescription = "WebAudio input";
    }

    virtual ~WebAudioSource() {
        Stop();
    }

    bool Start(int sampleRate, int frameSize) {
        m_sampleRate = sampleRate;
        // Additional initialization if needed
        return true;
    }

    void Stop() {
        // Cleanup if necessary
    }

    bool ReadAudio(Sample *buffer, int size) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (size > (int)m_samples.size()) {
            return false; // Not enough samples available
        }

        // Copy samples to output buffer
        for (int i = 0; i < size; i++) {
            buffer[i] = m_samples[i];
        }

        m_samples.erase(m_samples.begin(), m_samples.begin() + size);
        return true;
    }

    virtual const std::string &GetDescription() const override {
        return mDescription; // Return a description of the audio source
    }

    virtual void ReadAudioFrame(float dt, SampleBuffer<Sample> &outSamples) override {
        // Implementation for reading audio frame
    }

    virtual void Reset() override {
        // Implementation for resetting the audio source
    }

    void PushSamples(const std::vector<float>& left, const std::vector<float>& right) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < left.size(); ++i) {
            // Convert float samples to Sample structure
            m_samples.push_back(Sample(left[i], right[i]));
        }
    }

    // Factory method to create an instance of WebAudioSource
    static IAudioSourcePtr Create() {
        int sampleRate = 44100;
        auto source = std::make_shared<WebAudioSource>();
        source->Start(sampleRate, 1024); // Start with the determined sample rate and frame size
        return source;
    }

private:
    void InitAudioWorklet() {
        // This function should set up the audio worklet in JavaScript
        EM_ASM({
            const audioContext = new (window.AudioContext || window.webkitAudioContext)();
            audioContext.audioWorklet.addModule('audioWorklet.js').then(() => {
                const audioNode = new AudioWorkletNode(audioContext, 'audio-processor');
                audioNode.port.onmessage = (event) => {
                    // Send the audio samples to the WebAudioSource
                    const leftSamples = event.data.left;
                    const rightSamples = event.data.right;

                    // Call the C++ function to push samples
                    _push_samples(leftSamples, rightSamples);
                };
                audioNode.connect(audioContext.destination);
            });
        });
    }

    // Emscripten function to push samples from JavaScript
    static void PushSamples(float* left, float* right, int size) {
        std::vector<float> leftVec(left, left + size);
        std::vector<float> rightVec(right, right + size);
        instance->PushSamples(leftVec, rightVec);
    }

    std::vector<Sample> m_samples;
    std::mutex m_mutex;
    int m_sampleRate;

    // Static instance for callback
    static WebAudioSource* instance;
};

// Initialize the static instance
WebAudioSource* WebAudioSource::instance = nullptr;