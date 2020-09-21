"""
This is a list of all of the Formalizer core library components that are
independently configurable.

The `fzsetup.py` core tool can be used to set up the configuration
directories for these, and for executables.

Note: The `coreconfigurable` variable is a tuple instead of a list, which is much
      like a macro.
""" 
coreconfigurable = (
    'error',
    'fzpostgres',
    'standard'
    )
    