#pragma once
#include <Geode/Geode.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/binding/MusicDownloadManager.hpp>
#include <vector>
#include <string>
#include <thread>
#include <cmath>
#include <algorithm>
#include <memory>
#include <cstring>

using namespace geode::prelude;

class AudioWaveformManager {
public:
    static std::string getCurrentSongPath() {
        auto editor = LevelEditorLayer::get();
        if (!editor || !editor->m_level) return "";

        std::string path;
        if (editor->m_level->m_songID > 0) {
            path = MusicDownloadManager::sharedState()->pathForSong(editor->m_level->m_songID);
        } else {
            path = editor->m_level->getAudioFileName();
        }
        return CCFileUtils::get()->fullPathForFilename(path.c_str(), false);
    }

    static std::string getCurrentSongTitle() {
        auto editor = LevelEditorLayer::get();
        if (!editor || !editor->m_level) return "Unknown Song";

        if (editor->m_level->m_songID > 0) {
            return fmt::format("Song ID: {}", editor->m_level->m_songID);
        }
        return editor->m_level->getAudioFileName();
    }

    static void extractSamplesAsync(
        const std::string& songPath,
        int targetWidth,
        std::function<void(std::vector<float>, float)> onComplete
    ) {
        if (songPath.empty()) {
            if (onComplete) onComplete({}, 0.f);
            return;
        }

        std::thread([songPath, targetWidth, onComplete]() {
            auto* engine = FMODAudioEngine::sharedEngine();
            if (!engine || !engine->m_system) {
                Loader::get()->queueInMainThread([onComplete]() {
                    if (onComplete) onComplete({}, 0.f);
                });
                return;
            }

            FMOD::Sound* sound = nullptr;
            const FMOD_MODE mode = FMOD_DEFAULT | FMOD_OPENONLY;
            auto result = engine->m_system->createSound(songPath.c_str(), mode, nullptr, &sound);
            if (result != FMOD_OK || !sound) {
                Loader::get()->queueInMainThread([onComplete]() {
                    if (onComplete) onComplete({}, 0.f);
                });
                return;
            }

            unsigned int lengthMs = 0;
            sound->getLength(&lengthMs, FMOD_TIMEUNIT_MS);
            float songLength = static_cast<float>(lengthMs) / 1000.f;

            FMOD_SOUND_FORMAT format;
            int channels = 0;
            sound->getFormat(nullptr, &format, &channels, nullptr);

            unsigned int rawDataSize = 0;
            unsigned int sampleCount = 0;
            sound->getLength(&rawDataSize, FMOD_TIMEUNIT_PCMBYTES);
            sound->getLength(&sampleCount, FMOD_TIMEUNIT_PCM);

            if (channels <= 0) channels = 2;
            if (sampleCount <= 0) sampleCount = rawDataSize / 4;
            unsigned int bytesPerSample = (sampleCount > 0) ? (rawDataSize / sampleCount) : 4;

            if (rawDataSize == 0 || rawDataSize > 120 * 1024 * 1024) {
                sound->release();
                Loader::get()->queueInMainThread([onComplete]() {
                    if (onComplete) onComplete({}, 0.f);
                });
                return;
            }

            std::unique_ptr<uint8_t[]> rawData;
            try {
                rawData = std::make_unique<uint8_t[]>(rawDataSize);
            } catch (...) {
                sound->release();
                Loader::get()->queueInMainThread([onComplete]() {
                    if (onComplete) onComplete({}, 0.f);
                });
                return;
            }
            unsigned int readCount = 0;
            sound->readData(rawData.get(), rawDataSize, &readCount);
            sound->release();

            bytesPerSample /= channels;
            if (bytesPerSample <= 0) bytesPerSample = 2;

            const auto getSampleAt = [&](int sIdx) -> float {
                if (sIdx < 0 || sIdx >= static_cast<int>(sampleCount)) return 0.f;
                size_t dIdx = static_cast<size_t>(sIdx) * bytesPerSample * channels;
                if (dIdx + bytesPerSample * channels > rawDataSize) return 0.f;

                float sample = 0.f;
                for (int ch = 0; ch < channels; ++ch) {
                    size_t chIdx = dIdx + ch * bytesPerSample;
                    switch (format) {
                        case FMOD_SOUND_FORMAT_PCM8: {
                            int8_t val = static_cast<int8_t>(rawData[chIdx]);
                            sample += val / 128.f;
                            break;
                        }
                        case FMOD_SOUND_FORMAT_PCM16: {
                            int16_t val = static_cast<int16_t>(rawData[chIdx] | (rawData[chIdx + 1] << 8));
                            sample += val / 32768.f;
                            break;
                        }
                        case FMOD_SOUND_FORMAT_PCM24: {
                            int32_t val = (rawData[chIdx] << 8) | (rawData[chIdx + 1] << 16) | (rawData[chIdx + 2] << 24);
                            sample += (val >> 8) / 8388608.f;
                            break;
                        }
                        case FMOD_SOUND_FORMAT_PCM32: {
                            int32_t val = rawData[chIdx] | (rawData[chIdx + 1] << 8) | (rawData[chIdx + 2] << 16) | (rawData[chIdx + 3] << 24);
                            sample += val / 2147483648.f;
                            break;
                        }
                        case FMOD_SOUND_FORMAT_PCMFLOAT: {
                            float val = 0.f;
                            std::memcpy(&val, &rawData[chIdx], sizeof(float));
                            sample += val;
                            break;
                        }
                        default:
                            break;
                    }
                }
                return sample / channels;
            };

            std::vector<float> samples;
            samples.reserve(targetWidth);

            int windowSize = std::max(1, static_cast<int>(static_cast<float>(sampleCount) / targetWidth));
            for (int x = 0; x < targetWidth; ++x) {
                int center = static_cast<int>(static_cast<float>(x) / targetWidth * sampleCount);
                float peak = 0.f;
                int halfWin = std::max(1, windowSize / 2);
                for (int i = -halfWin; i <= halfWin; ++i) {
                    peak = std::max(peak, std::abs(getSampleAt(center + i)));
                }
                samples.push_back(std::clamp(peak, 0.f, 1.f));
            }

            Loader::get()->queueInMainThread([onComplete, samples = std::move(samples), songLength]() {
                if (onComplete) onComplete(samples, songLength);
            });
        }).detach();
    }
};
