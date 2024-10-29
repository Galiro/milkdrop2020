// src/audio/WebAudioSource.cpp
#include "IAudioSource.h"
#include <vector>
#include <mutex>
#include <emscripten.h>
#include <emscripten/webaudio.h>
#include <emscripten/wasm_worker.h>
#include <memory>
#include <stdio.h>
#include "Sample.h"


struct WebAudioComponents {
    EMSCRIPTEN_WEBAUDIO_T m_audioContext;
    EMSCRIPTEN_AUDIO_WORKLET_NODE_T m_audioWorklet;
    std::vector<Sample> m_samples;
};

class WebAudioSource : public IAudioSource {
    std::string mDescription;

    EMSCRIPTEN_WEBAUDIO_T m_audioContext;
    WebAudioWorkletProcessorCreateOptions opts;

    static constexpr size_t WORKLET_STACK_SIZE = 131072;  // Doubled the size to 128 KB
    alignas(16) char m_workletStack[WORKLET_STACK_SIZE];

    WebAudioComponents webAudioComponents;

public:
    EMSCRIPTEN_AUDIO_WORKLET_NODE_T m_audioWorklet;

    WebAudioSource() : m_samples(), m_sampleRate(44100) {
        m_audioContext = emscripten_create_audio_context(0);

        const size_t alignedStackSize = (sizeof(m_workletStack) / 16) * 16;

        std::cout << "Init Sample Size: " << webAudioComponents.m_samples.size() << std::endl;

        emscripten_start_wasm_audio_worklet_thread_async(m_audioContext, m_workletStack, WORKLET_STACK_SIZE, &audioThreadInitialized, &webAudioComponents);

        mDescription = "WebAudio input";
    }

    virtual ~WebAudioSource() {
        Stop();
    }

    virtual const std::string &GetDescription() const override {
        return mDescription; // Return a description of the audio source
    }

    virtual void ReadAudioFrame(float dt, SampleBuffer<Sample> &samples) override {
        samples.SetSampleRate((float)m_sampleRate);

        // samples.clear();
        // samples.reserve(webAudioComponents.m_samples.size());
        
        // for (const auto& sample : webAudioComponents.m_samples) {
        //     samples.push_back(sample);
        // }

        
        int total = (int)((float)m_sampleRate * dt);
        int avail = std::min(total, (int)webAudioComponents.m_samples.size());

  //      printf("ReadAudioFrame %d %d\n", (int)mSamples.size(),  total );

        // return samples from buffer
        samples.Assign(webAudioComponents.m_samples.begin(), webAudioComponents.m_samples.begin() + avail);
        webAudioComponents.m_samples.erase(webAudioComponents.m_samples.begin(), webAudioComponents.m_samples.begin() + avail);
        
        
        // pad end with empty samples
        while (samples.size() < total)
        {
            samples.push_back(Sample(0,0));
        }
        
        if (webAudioComponents.m_samples.size() > 2000)
            webAudioComponents.m_samples.clear();
    }

    virtual void Reset() override {
        // Implementation for resetting the audio source
    }

    bool Start(int sampleRate, int frameSize) {
        m_sampleRate = sampleRate;
        // Additional initialization if needed
        return true;
    }

    void Stop() {
        // Cleanup if necessary
    }

    // Factory method to create an instance of WebAudioSource
    static IAudioSourcePtr Create() {
        int sampleRate = 44100;
        auto source = std::make_shared<WebAudioSource>();
        source->Start(sampleRate, 1024); // Start with the determined sample rate and frame size
        return source;
    }

    static void audioThreadInitialized(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void* userData) {
        if (!success) return;

        WebAudioComponents* components = static_cast<WebAudioComponents*>(userData);
        components->m_audioContext = audioContext;
    
        WebAudioWorkletProcessorCreateOptions opts = {
          .name = "audio-analyzer",
        };
    
        emscripten_create_wasm_audio_worklet_processor_async(audioContext, &opts, &audioWorkletProcessorCreated, userData);
    }

    static void audioWorkletProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void* userData) {
        if (!success) return;

        WebAudioComponents* components = static_cast<WebAudioComponents*>(userData);

        int outputChannelCounts[2] = { 1, 1 };

        EmscriptenAudioWorkletNodeCreateOptions options = {
            .numberOfInputs = 1,
            .numberOfOutputs = 1,
            .outputChannelCounts = outputChannelCounts
        };
        
        std::cout << "Sample Size: " << components->m_samples.size() << std::endl;
        

        components->m_audioWorklet = emscripten_create_wasm_audio_worklet_node(audioContext, "audio-analyzer", &options, &defaultAudioProcessor, userData);
        components->m_audioContext = audioContext;

        initializeDOM(userData);
    }


    static void MessageReceivedOnMainThread() {
        
    }
    static bool defaultAudioProcessor(int numInputs, const AudioSampleFrame* inputs,
                                                int numOutputs, AudioSampleFrame* outputs,
                                                int numParams, const AudioParamFrame* params,
                                                void* userData) {
        WebAudioComponents* components = static_cast<WebAudioComponents*>(userData);

        // Default bypass implementation
        for (int i = 0; i < numOutputs; ++i) {
            const AudioSampleFrame& input = (i < numInputs) ? inputs[i] : AudioSampleFrame{0, nullptr};
            AudioSampleFrame& output = outputs[i];
            
            for (int channel = 0; channel < output.numberOfChannels; ++channel) {
                for (int sample = 0; sample < 128; ++sample) {
                    if (input.data) {
                        output.data[channel * 128 + sample] = input.data[channel * 128 + sample];

                        // if (input.data[channel * 128 + sample] >  0.00000000000 || input.data[channel * 128 + sample] < 0.000000000000) {
                        //     EM_ASM({
                        //         console.log("Sample %d, Channel %d: %f", $0, $1, $2);
                        //     }, sample, channel, input.data[channel * 128 + sample]);
                        // }

                        components->m_samples.push_back(Sample::Convert(input.data[channel * 128 + sample], 1.0f));
                        
                    } else {
                        EM_ASM({
                            console.log("No sample");
                        });
                    }
                }
            }
        }



        return true;
    }

    static void initializeDOM(void* userData) {
        WebAudioComponents* components = static_cast<WebAudioComponents*>(userData);
        
        EM_ASM({
            var audioContext = emscriptenGetAudioObject($0);
            var processorContext = emscriptenGetAudioObject($1);

            console.log("processor", processorContext);
            console.log("audioContext", audioContext);

            var container = document.createElement('div');
            container.style.cssText = "position: absolute; top: 0; z-index: 10000";

            var input = document.createElement('input');
            input.setAttribute('type', 'file');
            input.setAttribute('accept', 'audio/*');

            var audioElement = document.createElement('audio');
            audioElement.controls = true;
            container.appendChild(input);
            container.appendChild(audioElement);
            document.body.appendChild(container);

            input.onchange = function(e) {
                var file = e.target.files[0];
                var reader = new FileReader();
                reader.onload = function(e) {
                    console.log("context", audioContext);

                    audioElement.src = e.target.result;
                    audioElement.onloadedmetadata = function() {
                        var source = audioContext.createMediaElementSource(audioElement);
                        source.connect(processorContext).connect(audioContext.destination);
                        console.log(audioContext);
                        console.log(source);
                        console.log(processorContext);

                        console.log(audioContext.destination);
                    };
                };
                reader.readAsDataURL(file);
            };

            var startButton = document.createElement('button');
            startButton.innerHTML = 'Toggle playback';
            document.body.appendChild(startButton);

            startButton.onclick = function() {
                if (audioContext.state !== 'running') {
                    audioContext.resume();
                } else {
                    audioContext.suspend();
                }
            };
        }, components->m_audioContext, components->m_audioWorklet);
    }

private:
    std::vector<Sample> m_samples;
    std::mutex m_mutex;
    int m_sampleRate;
    size_t m_pos = 0;
    float m_sampleScale = 1.0f;



    // Static instance for callback
    static WebAudioSource* instance;
};

// Initialize the static instance
WebAudioSource* WebAudioSource::instance = nullptr;