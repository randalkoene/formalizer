# fzsetup - Set up a Formalizer environment

The `fzsetup` utility is the authoritative method to prepare or refresh a Formalizer environment.

This includes:

- Creating a `formalizer` database if one does not already exist.
- Creating a user schema in the database, which defaults to `$USER`.
- Create the fz-group role, which defaults to `fz-$USER`.
- Add members to the fz-group role, starting with the user role (defaulting to `$USER`) and the httpd CGI user role (defaulting to `www-data`).
- Give ownership of the user schema (`$USER`, by default) to the fz-group role.
- Create fresh (empty) versions of the Formalizer data structure tables.
- Grant authorizations to tables to specified roles, starting with the user role (defaulting to `$USER`) and the httpd CGI user role (defaulting to `www-data`).

-----

This utility is up-to-date to current Formalizer environment standards. Conversion from earlier format and protocol standards is done by separate conversion utilities, such as `tools/dil2graph`.
