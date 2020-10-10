"""
This is a list of all of the defined Formalizer components that can be called as
executables.

For general information about the Formalizer environment, use the fzinfo tool.
For information specific to a particular component, call the executable with
the "-h" option or see the Formalizer documentation.

This list differs from the one in the root Makefile in that:
  - It contains only the executable call names of the executable components
    (while the list in the Makefile expands to the full paths of components).
  - It contains only principal command line available executables for the
    Formalizer environment, specifically excluding:
      - Executables exclusively available as CGI handlers.
      - Executables exclusively provided for (temporary) active cross-
        compatible operation (from the formalizer/tools/compat sources).

Note: The `executables` variable is a tuple instead of a list, which is much
      like a macro.
""" 
executables = (
    'fzbackup.py',
    'fzguide.system',
    'fzinfo.py',
    'fzlog',
    'fzquerypq',
    'fzserverpq',
    'fzsetup.py',
    'dil2graph',
    'graph2dil',
    'boilerplate',
    'fzbuild.py',
    'fzgraphhtml',
    'fzloghtml',
    'nodeboard',
    'earlywiz.py',
    'requestmanual.py'
    )
    