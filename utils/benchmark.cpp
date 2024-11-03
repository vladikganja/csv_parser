#include "benchmark.h"

namespace ozma {

namespace benchmark {

bool hdrPercentilesPrint(
    struct hdr_histogram* h, FILE* stream, int32_t ticksPerHalfDistance, double maxPercentile) {
    const auto headFormat = "%12s %12s %12s %12s\n\n";
    const auto formatStr = "%s%d%s";
    char lineFormat[25];
    snprintf(lineFormat, 25, formatStr, "%12.", 3, "f %12f %12d %12.2f\n");
    static const char footer[] = "#[Mean    = %12.3f, StdDeviation   = %12.3f]\n"
                                 "#[Max     = %12.3f, Total count    = %12ld]\n"
                                 "#[Buckets = %12d, SubBuckets     = %12d]\n";
    struct hdr_iter iter;
    struct hdr_iter_percentiles* percentiles;

    hdr_iter_percentile_init(&iter, h, ticksPerHalfDistance);

    if (fprintf(stream, headFormat, "Value (ns)", "Percentile", "TotalCount", "%") < 0) {
        return EXIT_FAILURE;
    }

    percentiles = &iter.specifics.percentiles;
    while (hdr_iter_next(&iter)) {
        double value = iter.highest_equivalent_value;
        double percentile = percentiles->percentile / 100.0;
        int64_t totalCount = iter.cumulative_count;
        if (fprintf(stream, lineFormat, value, percentile, totalCount, percentiles->percentile) <
            0) {
            return EXIT_FAILURE;
        }
        if (percentile > maxPercentile) {
            break;
        }
    }

    double mean = hdr_mean(h);
    double stddev = hdr_stddev(h);
    double max = hdr_max(h);

    if (fprintf(
            stream,
            footer,
            mean,
            stddev,
            max,
            h->total_count,
            h->bucket_count,
            h->sub_bucket_count) < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

}   // namespace benchmark

}   // namespace ozma
