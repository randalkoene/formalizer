#!/usr/bin/python3
#
# Copyright {{ thedate }} Randal A. Koene
# License TBD

"""
A description of {{ this }}.py library module. See, for example, the description of
core/lib/Graphtypes.py.

"""

class Some_Class:
    """Class desription."""
    some_var = None         ##< this is a parameter of the class

    def __init__(self, _some_init_var):
        if (_some_init_var):
            self.some_var = _some_init_var
        
