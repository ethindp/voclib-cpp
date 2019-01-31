#pragma once
#define VOCLIB_IMPLEMENTATION
#include <voclib.h>
#include <exception>
#include <vector>

class Vocoder {
public:
Vocoder(unsigned char bands, unsigned char filters_per_band, unsigned int sample_rate) {
if (!voclib_initialize(this->voc_instance, bands, filters_per_band, sample_rate)) {
throw std::runtime_error("A parameter (bands, filters per band, or sample rate) was outside of its range.");
}
}

std::vector<float> Process(const float* carrier_buffer, const float* modulator_buffer, unsigned int frames) {
// We allocate a larger size than we most probably need to ensure that we have enough space since this could verry easily go out of bounds.
// TODO: determine if this allocates too much memory.
std::vector<float> buf(sizeof(carrier_buffer)*sizeof(modulator_buffer));
if (!voclib_process(this->voc_instance, carrier_buffer, modulator_buffer, buf.data(), frames)) {
throw std::runtime_error("Process: one or more parameters are not valid.");
} else {
return buf;
}
}

// Nice little compatibility version, if you so choose to use it.
std::vector<float> Process(const std::vector<float> carrier_buffer, const std::vector<float> modulator_buffer, unsigned int frames) {
std::vector<float> buf(sizeof(carrier_buffer)+sizeof(modulator_buffer));
if (!voclib_process(this->voc_instance, carrier_buffer.data(), modulator_buffer.data(), buf.data(), frames)) {
throw std::runtime_error("Process: one or more parameters are not valid.");
} else {
return buf;
}
}

void Reset() {
voclib_reset_history(this->voc_instance);
}

void SetReactionTime(float reaction_time) {
if (!voclib_set_reaction_time(this->voc_instance, reaction_time)) {
throw std::runtime_error("Invalid reaction time! Range: 0.002-2.0");
}
}

float GetReactionTime() {
return voclib_get_reaction_time(this->voc_instance);
}

void SetFormantShift(float formant_shift) {
if (!voclib_set_formant_shift(this->voc_instance, formant_shift)) {
throw std::runtime_error("Formant shift is not valid. Range: 0.25-4.0");
}
}

float GetFormantShift() {
return voclib_get_formant_shift(this->voc_instance);
}
private:
voclib_instance* voc_instance;
};
