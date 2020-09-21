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
- Creating a directory tree for `config` files. Existing configuration directories and files are not removed or modified.

-----

## Before running this program

1. Make sure that the Formalizer source tree is accessible. (Confirm that its root is in the expected place as per `sourceroot` in the `fzsetup.py` configuration, and as per `FORMALIZERPATH` in the root `Makefile`.)
2. Make sure that the directory where the Formalizer web interaction directory tree will be placed is accessible and writable for this user. (For example, you may have to run:`sudo chown $(USER):$(USER) /var/www/html`.)

(Possibly put some more here, right now the program already properly deals with the situation where its own configuration file does not yet exist.)

-----

This utility is up-to-date to current Formalizer environment standards. Conversion from earlier format and protocol standards is done by separate conversion utilities, such as `tools/dil2graph`.
