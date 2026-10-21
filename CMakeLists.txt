cmake_minimum_required(VERSION 3.15)
project(FREE_TEK_PLUGIN VERSION 1.0.0)

add_subdirectory(JUCE)

juce_add_plugin(FREE_TEK_PLUGIN
    COMPANY_NAME "FREE-TEK"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Ftek
    PLUGIN_CODE Ftk1
    FORMATS VST3
    PRODUCT_NAME "FREE-TEK Compressor Distortion"
)

target_sources(FREE_TEK_PLUGIN
    PRIVATE
        PluginProcessor.cpp
        PluginEditor.cpp
)

target_compile_definitions(FREE_TEK_PLUGIN
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(FREE_TEK_PLUGIN
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
