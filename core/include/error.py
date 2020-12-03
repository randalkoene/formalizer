# Copyright 2020 Randal A. Koene
# License TBD

"""
This header file declares the Formalizer Core Error codes.

Versioning is based on https://semver.org/. See coreversion.hpp for more.
"""

exit_status_code = {
    0 : 'exit_ok',
    1 : 'exit_general_error',
    2 : 'exit_database_error',
    3 : 'exit_unknown_option',
    4 : 'exit_cancel',
    5 : 'exit_conversion_error',
    6 : 'exit_DIL_error',
    7 : 'exit_unable_to_stack_clean_exit',
    8 : 'exit_command_line_error',
    9 : 'exit_file_error',
    10 : 'exit_missing_parameter',
    11 : 'exit_missing_data',
    12 : 'exit_bad_config_value',
    13 : 'exit_resident_graph_missing',
    14 : 'exit_bad_request_data',
    15 : 'exit_communication_error'
}
