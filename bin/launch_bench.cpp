#include "launch_bench.h"

#include "common.h"
#include "logger.h"
#include "benchmark.h"

#include "fastcsv/csv.h"

#include "rapidcsv/src/rapidcsv.h"

#include "vinces/single_include/csv.hpp"
#include <chrono>

namespace ozma {

namespace {

using Fastcsv = io::CSVReader<4, io::trim_chars<' ', '\t'>, io::double_quote_escape<',', '\"'>>;
using Rapidcsv = rapidcsv::Document;
using Vinces = csv::CSVReader;

enum class Side { Asks, Bids };

struct Order {
    double price;
    double size;
};

struct BTCUSDT {
    uint64_t t;
    uint64_t u;
    std::vector<Order> orders;
    Side side;
};

enum class Reader { Fastcsv, Rapidcsv, Vinces };
DECLARE_ENUM(Reader, 3, Fastcsv, Rapidcsv, Vinces);

template <typename Reader>
class Parser {

};

}   // namespace



void launch() {
    {
        for (int i = 100; i; i--) {
            Fastcsv in("./csv/data.csv");
            std::string body;
            int32_t id;
            int32_t b1, b2;
            
            BENCH_START(Reader, Fastcsv);
            while (in.read_row(body, id, b1, b2)) {
                //INFO() << id;
            }
            BENCH_END(Reader, Fastcsv);
        }
        INFO() << BENCH_DISTR(Reader, Fastcsv);
    }
    return;

    {
        Rapidcsv doc("./csv/data.csv");
        BENCH_START(Reader, Rapidcsv);
        for (size_t i = 0; i < doc.GetRowCount(); i++) {
            UNUSED(doc.GetCell<std::string>(1, i));
        }
        BENCH_END(Reader, Rapidcsv);
        INFO() << ReaderStr(Reader::Rapidcsv) << ": "
               << BENCH_AVG(Reader, Rapidcsv, std::chrono::microseconds).count();
    }

    {
        Vinces reader("./csv/data.csv");
        BENCH_START(Reader, Vinces);
        for (csv::CSVRow& row : reader) {
            UNUSED(row);
            //INFO() << row[1].get();
        }
        BENCH_END(Reader, Vinces);
        INFO() << ReaderStr(Reader::Vinces) << ": " << BENCH_AVG(Reader, Vinces).count();
    }
}

}   // namespace ozma
