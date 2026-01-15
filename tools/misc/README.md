# Miscellaneous

This category includes both Prototypes for Testing, as well as tools that are fairly
user specific and less likely to be standard tools of any Formalizer installation.

## Testing

A collection of (mostly tiny) prototypes developed for testing purposes. These are not
formally part of the Formalizer core or tools module structure.

## User

A collection of tools that perform useful actions that may be integrated in a specific
Formalizer setup, but that are not as general-purpose as other tool categories.

## How to use these

1. Copy the executable scripts to the CGI directory (typically `/usr/lib/cgi-bin/`).
2. Copy the `.html` files to a web-visible directory (e.g. `/var/www/html/formalizer/`).

Note that this is typically done through inclusion in the top Makefile in categories
such as EXECUTABLES, SYMBIN and CGIEXE.

----

Randal A. Koene, 2020
