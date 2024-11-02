#include "launch_bench.h"

#include "utils/logger.h"

#include <boost/program_options.hpp>

namespace opt = boost::program_options;

int main(int argc, char* argv[]) try {
    INIT_LOGGER({ ozma::Logger::Out::Stdout, ozma::Logger::Out::File });

    opt::options_description desc("all options");
    opt::variables_map vm;

    desc.add_options()("help,h", "Show help");

    opt::store(opt::parse_command_line(argc, argv, desc), vm);
    opt::notify(vm);

    if (vm.contains("help")) {
        INFO() << desc;
        return EXIT_SUCCESS;
    }

    ozma::launch();

    return EXIT_SUCCESS;
} catch (std::exception& ex) {
    ERROR() << ex.what();
    return EXIT_FAILURE;
}
