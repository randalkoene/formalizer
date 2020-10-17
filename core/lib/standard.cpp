// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <unistd.h>
#include <iostream> //*** only here for std::cin in key_press(), remove if that is moved elsewhere
//#include <utility>

// core
#include "error.hpp"
#include "standard.hpp"
#include "Graphtypes.hpp"

namespace fz {

formalizer_base_streams base; // The standard base streams.

the_standard_object standard; ///< The standard object for Formalizer programs.

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
    VERYVERBOSEOUT("Resolving exit hook: error_summary_wrapper\n");
    if ((base.out) && (!standard.quiet)) {
        ERRWARN_SUMMARY(*base.out);
    }
}

/// This does everyting that `fz::Clean_Exit()` does, and it is stacked.
void clean_exit_wrapper() {
    VERYVERBOSEOUT("Resolving exit hook: clean_exit_wrapper\n");
    ErrQ.output(ERRWARN_LOG_MODE);
    WarnQ.output(ERRWARN_LOG_MODE);
}


/// Set the runnablename once you have argv[0], typically in formalizer_standard_program::init().
void the_standard_object::set_name(std::string argv0) {
    standard.runnablename = argv0;
    std::string::size_type slashpos = standard.runnablename.find_last_of('/');
    if (slashpos != std::string::npos)
        standard.runnablename.erase(0,slashpos+1);
}

void the_standard_object::set_id(std::string _serverlongid) {
    server_long_id = _serverlongid;
}

/// (Almost) identical to regular exit(). (See details in standard.hpp.)
void the_standard_object::exit(int exit_code) {
    std::exit(exit_code);
}

/// Add additional steps to the exit() stack.
void the_standard_object::add_to_exit_stack(void (*func) (void), std::string label) {
    // *** could add a step here to remember a list of exit steps that can be reported
    VERYVERBOSEOUT("Adding exit hook: "+label+'\n');
    if (atexit(func)!=0) {
        ADDERROR(__func__,"unable to add clean exit function to exit stack");
        error_summary_wrapper();
        clean_exit_wrapper();
        exit(exit_unable_to_stack_clean_exit);
    }
}

int the_standard_object::completed_ok() {
    VERBOSEOUT(runnablename+" completed.\n");
    exit(exit_ok);
    return exit_ok;
}

/// Another set of exit options, these with potential messages.
int standard_exit_success(std::string veryverbose_message) {
    if (!veryverbose_message.empty()) {
        VERYVERBOSEOUT(veryverbose_message);
    }
    return standard.completed_ok();
}

int standard_exit_error(int exit_code, std::string error_message, const char * problem__func__) {
    ADDERROR(problem__func__, error_message);
    VERBOSEERR(error_message+'\n');
    standard.exit(exit_code);
    return exit_code;
}

int standard_exit(bool success, std::string veryverbose_message, int exit_code, std::string error_message, const char * problem__func__) {
    if (success)
        return standard_exit_success(veryverbose_message);
    
    return standard_exit_error(exit_code, error_message, problem__func__);
}


void the_standard_object::print_version() {
    FZOUT(runnablename+" "+server_long_id+'\n');
}

formalizer_standard_program::formalizer_standard_program(bool _usesconfig): uses_config(_usesconfig) {
    // Warning: Only put things here for which it does not matter if the
    // standard object is initialized later than a local object that
    // uses a derived class.
    COMPILEDPING(std::cout, "PING-formalizer_standard_program().1\n");
    standard.quiet = false;
    standard.veryverbose = false;
}

/// Add the init function of a `main_init_register` derived class to the `main_init_register` queue.
void formalizer_standard_program::add_registered_init(main_init_register * mir) { //bool (*initfunc) (void)) {
    init_register_stack.emplace_back(mir);//initfunc);
}

//*** It could be useful to replace the below with use of the templater.hpp methods.
void formalizer_standard_program::print_usage() {
    FZOUT("Usage: "+name()+" [-E <errfile>] [-W <warnfile>] [-q|-V]"+add_usage_top+'\n');
    FZOUT("       "+name()+" <-v|-h>\n\n");

    if (usage_head.size()>0) {
        for (const auto& ustr : usage_head)
            FZOUT(ustr);
        FZOUT("\n");
    }

    FZOUT("  Options:\n");

    usage_hook();

    FZOUT("    -E send error output to <errfile> (STDOUT=standard output)\n");
    FZOUT("    -W send warning output to <warnfile> (STDOUT=standard output)\n");
    FZOUT("    -q set quieter running\n");
    FZOUT("    -V set very verbose running\n");
    FZOUT("    -v print version info\n");
    FZOUT("    -h print this help\n\n")

    if (usage_tail.size()>0) {
        for (const auto& ustr : usage_tail)
            FZOUT(ustr);
        FZOUT("\n");
    }

    FZOUT(id()+"\n\n");
}

void formalizer_standard_program::commandline(int argc, char *argv[]) {
    int optionindex;
    add_option_args += "hqVvE:W:";

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
            standard.veryverbose = false;
            standard.quiet = true;
            break;

        case 'V':
            standard.veryverbose = true;
            standard.quiet = false;
            break;

        case 'v':
            standard.print_version();
            exit(exit_ok);

        default:
            print_usage();
            exit(exit_ok);
        }
    }
}

void formalizer_standard_program::init(int argc, char *argv[], std::string version, std::string module, std::ostream * o, std::ostream * e) {
    ERRHERE(".top");
    // Do these here so that they also work for derived classes.s
    base.out = o;
    base.err = e;

    // Do these first! We want to be able to use these hooks for any
    // exits even during following init processing.
    // ***if (base.out) { // *** I can't remember why this conditional was here, so I'm removing it for now and I'll see if something breaks! I need better documentation!!!
    standard.add_to_exit_stack(&clean_exit_wrapper,"clean_exit_wrapper");
    // ***}
    standard.add_to_exit_stack(&error_summary_wrapper,"error_summary_wrapper");

    ERRHERE(".config");
    standard.set_name(argv[0]);

    if (uses_config) { // *** I think this has been replaced by the `configurable` method.
        // *** This is where the actual configuration call or hook should probably be!
        // (But we can't print the warning here, or else we can't suppress it with -q
        // due to needing to parse the command line first.)
    }
    ErrQ.init(); // this one uses `configbase` instead of `configurable` (it also takes care of WarnQ)
    if (!init_register_stack.empty()) { // call registered init functions
        for (const auto& initfuncptr : init_register_stack) {
            initfuncptr->init(); //(*initfuncptr)();
        }
    }

    // Note: Don't PRINT ANYTHING before parsing the command line in order to catch -q as needed!
    ERRHERE(".commandline");
    standard.set_id(module + " " + version + " (core v" + coreversion() + ")");
    commandline(argc,argv);
    VERBOSEOUT(id()+"\n\n");

    if (uses_config) { // *** Remove this warning once a configuration method is in use!
        VERBOSEOUT("** CONFIG NOTE: This standardized Formalizer component still needs a\n");
        VERBOSEOUT("**              standardized method of configuration. See Trello card\n");
        VERBOSEOUT("**              at https://trello.com/c/4B7x2kif.\n\n");
    }

    ERRHERE(".initreport");
    initialized = true;
    // standard.initialized = true; // *** do this here? it isn't really true but some variables are maintained either way

    VERYVERBOSEOUT("Very verbose output activated.\n");
    // Add this here, because the calls to `add_to_exit_stack()` above would not yet
    // have had a chance to turn on `standard.veryverbose` before checking
    // configuration or command line options.
    VERYVERBOSEOUT("Adding exit hook: clean_exit_wrapper\n");
    VERYVERBOSEOUT("Adding exit hook: error_summary_wrapper\n");
}

main_init_register::main_init_register(formalizer_standard_program & fsp) { //, bool (*initfunc) (void)) {
    fsp.add_registered_init(this); //initfunc);
}

} // namespace fz
