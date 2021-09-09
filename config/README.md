### About configuration files

Core libraries and Formalizer standardized programs expect configuration files arranged within a `.formalizer/config/this-component/` directory, where `this-component` is the library component or program in question. The configuration directories can be initialized by running `fzsetup.py -C`.

The format of the configuration files is is a subset of JSON, as follows:

```

    {
    "variable_name1" : "some string value",
    "variable_name2" : 157,
    "variable_name3" : "SOME_IDENTIFIER_OR_LABELED_SETTING"
    }

```

Reasons:

- It may pay off to remain close to a known standard when then is little performance benefit to be had by using one of the others.
- That format appears to be immediately loadable in Python as a `dict`.
- Colon is just as good as equal sign.
- Comma is just as good as semicolon.
- By avoiding the recursive depth of regular JSON for now, parsing is kept very simple.

For more, see https://trello.com/c/4B7x2kif.
