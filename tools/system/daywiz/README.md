# Formalizer tool: daywiz

The `daywiz` tool is a wizard tool that helps carry out a set of System recommended
steps throughout the day.

## Files

1. `daywiz.py`: The principal DayWiz script. Displays and provides interface to DayWiz.
2. `consumed.py`: Generates a helpful table of known foods for consumption entry.
3. `metrics.py`: Use this to load metrics data, parse it, and produce useful output.
4. `nutrition.py`: (Under construction. Intended to help with nutrition planning.)
5. `score.py`: Generate evaluated and scored output for the day and recent days.
6. `wiztable.py`: Contains the definition of the table on the left-hand side of the DayWiz page.

## Additional dependencies

1. `core/include/fznutrition.py`: Data file for known foods, categories, etc.
2. `core/include/fzhtmlpage.py`: HTML page builder template classes.

## More

See https://trello.com/c/JssbodOF .

Also, note that the metrics defined and calculated in the Formalizer component
`tools/system/metrics` are separate from those in DayWiz.

--
Randal A. Koene, 2023
