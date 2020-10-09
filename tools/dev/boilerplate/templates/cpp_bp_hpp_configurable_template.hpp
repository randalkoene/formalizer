
class {{ th }}_configurable: public configurable {
public:
    {{ th }}_configurable(formalizer_standard_program & fsp): configurable("{{ this }}", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    //std::string example_par;   ///< example of configurable parameter
};
