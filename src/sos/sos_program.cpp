#include "sos_program.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>

#if defined(XBS_WIN32_DESKTOP)
#include <windows.h>
#endif

namespace SOS {

namespace
{

const char* opName(int op)
{
    switch (op)
    {
    case F_REST: return "REST";       case F_NOTE: return "NOTE";
    case F_JUMPTO: return "JUMPTO";   case F_LOOP: return "LOOP";
    case F_ENDLOOP: return "ENDLOOP"; case F_PATCH: return "PATCH";
    case F_PAN: return "PAN";         case F_MUX: return "MUX";
    case F_DEMUX: return "DEMUX";     case F_VOLUME: return "VOLUME";
    case F_XPOSE: return "XPOSE";     case F_XSET: return "XSET";
    case F_SLUR: return "SLUR";       case F_RING: return "RING";
    case F_CLOCKSET: return "CLOCKSET"; case F_END: return "END";
    case F_FILTERINC: return "FILTERINC"; case F_FILTERSET: return "FILTERSET";
    default: return "?";
    }
}

bool fail(std::string* error, const char* fmt, ...)
{
    if (error)
    {
        va_list ap;
        va_start(ap, fmt);

        va_list ap_testSize;
        va_copy(ap_testSize, ap);

        int size = std::vsnprintf(nullptr, 0, fmt, ap_testSize);

        va_end(ap_testSize);

        if (size >= 0)
        {
            error->resize(static_cast<size_t>(size));
            std::vsnprintf(error->data(), error->size() + 1, fmt, ap);
        }
#if !defined(XBS_WIN32_DESKTOP)
        std::puts(error->c_str());
#else
        MessageBoxA(nullptr, error->c_str(), "SOS - Error", MB_ICONERROR + MB_OK);
#endif

        va_end(ap);

        if (size < 0) error->clear();
    }

    return false;
}

bool validateTrack(const Program& prog, size_t track, std::string* error)
{
    const std::vector<int16_t>& code = prog.tracks[track].code;
    size_t pc = 0;
    size_t depth = 0;

    while (pc < code.size())
    {
        const int op = code[pc];
        const int arity = opArity(op);

        if (arity < 0)
            return fail(error, "track %zu, word %zu: opcode %d (%s) is unknown or not implemented",
                        track, pc, op, opName(op));

        if (pc + 1 + (size_t)arity > code.size())
            return fail(error, "track %zu, word %zu: %s is missing its operand(s)",
                        track, pc, opName(op));

        switch (op)
        {
        case F_PATCH:
        {
            const int idx = code[pc + 1];
            if (idx < 0 || (size_t)idx >= prog.patches.size())
                return fail(error, "track %zu, word %zu: PATCH %d out of range (program has %zu patches)",
                            track, pc, idx, prog.patches.size());
            break;
        }
        case F_LOOP:
            if (depth >= MAX_LOOP_DEPTH)
                return fail(error, "track %zu, word %zu: loops nested deeper than %zu",
                            track, pc, MAX_LOOP_DEPTH);
            depth++;
            break;
        case F_ENDLOOP:
            if (depth == 0)
                return fail(error, "track %zu, word %zu: ENDLOOP without matching LOOP", track, pc);
            depth--;
            break;
        case F_END:
            return true;
        default:
            break;
        }

        pc += 1 + (size_t)arity;
    }

    if (depth != 0)
        return fail(error, "track %zu: LOOP without matching ENDLOOP", track);

    return true;
}

}

bool Program::validate(std::string* error) const
{
    if (!std::isfinite(tickSeconds) || tickSeconds < 0.0001f || tickSeconds > 1.0f)
        return fail(error, "tickSeconds %g out of range (0.0001 .. 1.0)", tickSeconds);

    if (tracks.size() > (size_t)MAX_TRACKS)
        return fail(error, "%zu tracks, but the synth has %d channels", tracks.size(), MAX_TRACKS);

    if (patches.size() > 32767)
        return fail(error, "%zu patches, but a PATCH operand is a 16-bit index", patches.size());

    for (size_t i = 0; i < patches.size(); i++)
    {
        const std::vector<float>& d = patches[i].sample.data;
        if (!std::all_of(d.begin(), d.end(), [](float f) { return std::isfinite(f); }))
            return fail(error, "patch %zu: sample data contains NaN or infinity", i);
    }

    for (size_t i = 0; i < tracks.size(); i++)
    {
        const Pan& p = tracks[i].pan;
        if (!std::isfinite(p.left) || !std::isfinite(p.right) || p.left < 0.0f || p.right < 0.0f)
            return fail(error, "track %zu: pan gains must be finite and non-negative", i);

        if (!validateTrack(*this, i, error))
            return false;
    }

    return true;
}

}
