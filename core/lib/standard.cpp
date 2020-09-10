// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <unistd.h>
#include <iostream> //*** only here for std::cin in key_press(), remove if that is moved elsewhere
//#include <utility>

// core
#include "standard.hpp"
#include "Graphtypes.hpp"

namespace fz {

formalizer_base_streams base; // The standard base streams.

formalizer_standard_program standard; /// The standard object for Formalizer programs.

/**
 * Safer parsing of command line parameters.
 * 
 * This wrapper aims to reduce that ways in which parsing command line parameters can
 * lead to segfaults. (The getopt() function is not very safe in that regard.)
 * 
 * Note: This wrapper does use getopt() and getopt() is known to modify argv. Therefore,
 *       if you need the original argv, copy it before calling this functions. For example,
 *       with `std::vector<std::string> cmdargs; for (int i = 0; i < argc; ++i) cmdargs[i] = argv[i];`
 * 
 * @param argc is the argc from main().
 * @param argv is the argv from main().
 * @param options is the options list in the same format as for getopt().
 * @param optindcopy is a variable reference that receives a copy of optind as used by getopt().
 */
std::pair<int, std::string> safe_cmdline_options(int argc, char *argv[], std::string options, int &optindcopy) {
    optindcopy = 1;
    if ((argc < 1) || (options.empty()))
        return std::pair<int, std::string>(EOF, std::string(""));

    int c = getopt(argc, argv, options.c_str());
    optindcopy = optind;
    if ((c == EOF) || (optarg == nullptr))
        return std::pair<int, std::string>(c, std::string(""));

    std::string option_argument = optarg;
    return std::pair<int, std::string>(c, option_argument);
}

//*** It is not entirely clear if key_pause() should be here or in some stream utility set.
/// A very simple function to wait for ENTER to be pressed.
void key_pause() {
    FZOUT("...Presse ENTER to continue (or CTRL+C to exit).\n");
    std::string enterstr;
    std::getline(std::cin, enterstr);
}

/**
 * A wrapped version of ERRWARN_SUMMARY that can be stacked.
 * 
 * This can be suppressed either by setting base.out to nullptr
 * or by setting the standard.quiet flag. (Note that this flag
 * receives a copy of any formalizer_standard_program::quiet
 * when formalizer_standard_program::exit() is used.)
 */
void error_summary_wrapper() {
    if ((base.out) && (!standard.quiet)) {
        ERRWARN_SUMMARY(*base.out);
    }
}

/// This does everyting that `fz::Clean_Exit()` does, and it is stacked.
void clean_exit_wrapper() {
    ErrQ.output(ERRWARN_LOG_MODE);
    WarnQ.output(ERRWARN_LOG_MODE);
}

formalizer_standard_program::formalizer_standard_program(): quiet(false) {
    // Warning: Only put things here for which it does not matter if the
    // standard object is initialized later than a local object that
    // uses a derived class.
    COMPILEDPING(std::cout, "PING-formalizer_standard_program().1\n");
}

/// Almost identical to regular exit(). (See details in standard.hpp.)
void formalizer_standard_program::exit(int exit_code) {
    standard.quiet = quiet;
    std::exit(exit_code);
}

void formalizer_standard_program::print_version() {
    FZOUT(runnablename+" "+server_long_id+'\n');
}

//*** It could be useful to replace the below with use of the templater.hpp methods.
void formalizer_standard_program::print_usage() {
    FZOUT("Usage: "+runnablename+" [-E <errfile>] [-W <warnfile>] [-q]"+add_usage_top+'\n');
    FZOUT("       "+runnablename+" <-v|-h>\n\n");

    FZOUT("  Options:\n");

    usage_hook();

    FZOUT("    -E send error output to <errfile> (STDOUT=standard output)\n");
    FZOUT("    -W send warning output to <warnfile> (STDOUT=standard output)\n");
    FZOUT("    -q set quieter running\n");
    FZOUT("    -v print version info\n");
    FZOUT("    -h print this help\n\n")

    FZOUT(server_long_id+"\n\n");
}

void formalizer_standard_program::commandline(int argc, char *argv[]) {
    int optionindex;
    add_option_args += "hqvE:W:";

    while (true) {
        auto [c, option_arg] = safe_cmdline_options(argc, argv, add_option_args.c_str(), optionindex);
        if (c==EOF)
            break;

        if (options_hook(c, option_arg))
            continue; // in this case don't test below (which would go to the default)

        switch (c) {

        case 'E':
            ErrQ.set_errfilepath(option_arg);
            break;

        case 'W':
            WarnQ.set_errfilepath(option_arg);
            break;

        case 'q':
            quiet = true;
            standard.quiet = true; // visible to all
            break;

        case 'v':
            print_version();
            exit(exit_ok);

        default:
            print_usage();
            exit(exit_ok);
        }
    }
}

void formalizer_standard_program::init(int argc, char *argv[], std::string version, std::string module, std::ostream * o, std::ostream * e) {
    // Do these here so that they also work for derived classes.s
    base.out = o;
    base.err = e;

    if (base.out) {
        add_to_exit_stack(&clean_exit_wrapper);
    }
    add_to_exit_stack(&error_summary_wrapper);

    runnablename = argv[0];
    std::string::size_type slashpos = runnablename.find_last_of('/');
    if (slashpos != std::string::npos)
        runnablename.erase(0,slashpos+1);

    server_long_id = module + " " + version + " (core v" + coreversion() + ")";
    commandline(argc,argv);
    if (!quiet) { 
        FZOUT(server_long_id+"\n\n");
    }
    initialized = true;
}

int formalizer_standard_program::completed_ok() {
    if (!quiet) {
        FZOUT(runnablename+" completed.\n");
    }
    exit(exit_ok);
    return exit_ok;
}

} // namespace fz
