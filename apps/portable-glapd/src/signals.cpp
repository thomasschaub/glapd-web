#include "signals.h"

#ifndef EMSCRIPTEN

extern "C" {

// glapd_signals.h

void notify_about_to_check_candidate_primer_region(int current, int total)
{
}

void notify_about_to_check_primer_set_candidate(int num_targets, int current, int total)
{
}

void notify_found_primer_set_candidate_begin(const char* f3, const char* f2, const char* f1c, const char* b1c, const char* b2, const char* b3, const char* lf, const char* lb)
{
}

void notify_primer_set_candidate_can_be_used_for(const char* name)
{
}

void notify_found_primer_set_candidate_end()
{
}

// signals.h

void notify_about_to_start_phase(const char* phase)
{
}

}

#endif