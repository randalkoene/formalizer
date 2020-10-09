
{{ th }}_configbase::{{ th }}_configbase(): configbase("{{ this }}") {
}

bool {{ th }}_configbase::set_parameter(const std::string &parlabel, const std::string &parvalue) {
    //CONFIG_TEST_AND_SET_PAR(example_par, "examplepar", parlabel, parvalue);
    //CONFIG_TEST_AND_SET_FLAG(example_flag_funcenable, example_flag_funcdisable, "exampleflag", parlabel, parvalue);
    CONFIG_PAR_NOT_FOUND(parlabel);
}

// NOTE: Include the following in an initialization function (see error.cpp:Errors::init())
/*
    if (!config.load()) {
        const std::string configerrstr("Errors during "+config.get_configfile()+" processing");
        VERBOSEERR(configerrstr+'\n');
        ERRRETURNFALSE(__func__,configerrstr);
    }
*/
