#include "../frame_clock.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static int check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", name);
        return 0;
    }
    return 1;
}

static int test_supported_rates(void)
{
    static const uint32_t rates[] = { 60, 120, 240 };
    size_t i;

    for (i = 0; i < sizeof(rates) / sizeof(rates[0]); ++i) {
        MeleePc_FrameClock clock;
        uint64_t simulation = 0;
        uint64_t presentations = 0;

        if (!check(MeleePc_FrameClockInit(&clock, rates[i]), "supported rate initializes") ||
            !check(MeleePc_FrameClockAdvance(&clock, UINT64_C(1000000000),
                                             &simulation, &presentations),
                   "one second advances") ||
            !check(simulation == 60, "simulation remains 60 Hz") ||
            !check(presentations == rates[i], "presentation follows selected rate") ||
            !check(clock.simulation_phase == 0 && clock.presentation_phase == 0,
                   "whole-second phases return to zero")) {
            return 0;
        }
    }
    return 1;
}

static int test_partition_invariance(void)
{
    static const uint32_t rates[] = { 60, 120, 240 };
    static const uint64_t slices[] = {
        UINT64_C(1234567), UINT64_C(80000003), UINT64_C(700000001),
        UINT64_C(199999999), UINT64_C(101234567), UINT64_C(81765432),
        UINT64_C(45678901), UINT64_C(123456789), UINT64_C(133333333),
        UINT64_C(100000008)
    };
    size_t i;

    for (i = 0; i < sizeof(rates) / sizeof(rates[0]); ++i) {
        MeleePc_FrameClock clock;
        uint64_t simulation_total = 0;
        uint64_t presentation_total = 0;
        uint64_t elapsed_total = 0;
        size_t j;

        if (!MeleePc_FrameClockInit(&clock, rates[i])) return 0;
        for (j = 0; j < sizeof(slices) / sizeof(slices[0]); ++j) {
            uint64_t simulation = 0;
            uint64_t presentations = 0;
            if (!MeleePc_FrameClockAdvance(&clock, slices[j],
                                           &simulation, &presentations)) {
                return check(0, "partitioned advance succeeds");
            }
            simulation_total += simulation;
            presentation_total += presentations;
            elapsed_total += slices[j];
        }
        {
            MeleePc_FrameClock whole_clock;
            uint64_t whole_simulation = 0;
            uint64_t whole_presentations = 0;
            if (!MeleePc_FrameClockInit(&whole_clock, rates[i]) ||
                !MeleePc_FrameClockAdvance(&whole_clock, elapsed_total,
                                           &whole_simulation, &whole_presentations) ||
                !check(simulation_total == whole_simulation,
                       "simulation count is independent of update partition") ||
                !check(presentation_total == whole_presentations,
                       "presentation count is independent of update partition") ||
                !check(clock.simulation_phase == whole_clock.simulation_phase &&
                           clock.presentation_phase == whole_clock.presentation_phase,
                       "fractional phases are independent of update partition")) {
                return 0;
            }
        }
    }
    return 1;
}

static int test_rejections_and_extreme_elapsed(void)
{
    MeleePc_FrameClock clock;
    uint64_t simulation = UINT64_MAX;
    uint64_t presentations = UINT64_MAX;

    if (!check(!MeleePc_FrameClockInit(NULL, 60), "null clock rejected") ||
        !check(!MeleePc_FrameClockInit(&clock, 0), "zero rate rejected") ||
        !check(!MeleePc_FrameClockInit(&clock, 90), "unsupported rate rejected") ||
        !check(MeleePc_FrameClockInit(&clock, 240), "240 Hz initializes") ||
        !check(MeleePc_FrameClockAdvance(&clock, 0, &simulation, &presentations),
               "zero elapsed succeeds") ||
        !check(simulation == 0 && presentations == 0, "zero elapsed produces no work") ||
        !check(!MeleePc_FrameClockAdvance(&clock, 1, &simulation, &simulation),
               "aliased outputs rejected") ||
        !check(!MeleePc_FrameClockAdvance(&clock, 1, NULL, &presentations),
               "null output rejected")) {
        return 0;
    }

    clock.presentation_hz = 90;
    if (!check(!MeleePc_FrameClockAdvance(&clock, 1, &simulation, &presentations),
               "corrupted rate rejected")) return 0;

    if (!MeleePc_FrameClockInit(&clock, 240)) return 0;
    if (!check(MeleePc_FrameClockAdvance(&clock, UINT64_MAX,
                                         &simulation, &presentations),
               "maximum elapsed nanoseconds do not overflow") ||
        !check(simulation == (UINT64_MAX / UINT64_C(1000000000)) * 60 +
                                 ((UINT64_MAX % UINT64_C(1000000000)) * 60) /
                                     UINT64_C(1000000000),
               "extreme simulation count matches reference") ||
        !check(presentations == (UINT64_MAX / UINT64_C(1000000000)) * 240 +
                                    ((UINT64_MAX % UINT64_C(1000000000)) * 240) /
                                        UINT64_C(1000000000),
               "extreme presentation count matches reference")) {
        return 0;
    }
    return 1;
}

int main(void)
{
    if (!test_supported_rates() || !test_partition_invariance() ||
        !test_rejections_and_extreme_elapsed()) {
        return EXIT_FAILURE;
    }
    puts("PASS: frame clock 60/120/240 Hz, partition invariance, validation, and overflow bounds");
    return EXIT_SUCCESS;
}
