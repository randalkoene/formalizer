
class {{ th }}_configbase: public configbase {
public:
    err_configbase();

    virtual bool set_parameter(const std::string & parlabel, const std::string & parvalue);
};
