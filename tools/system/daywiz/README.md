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

## Database storage

The default version of `daywiz.py` now uses the Formalizer database to store and
retrieve data.

The data is stored in multiple tables by datestamp key. The content of each table is
in the form of a JSON string containing the relevant data belonging to that table for
that particular day.

The nature of the content is transparent to the database, because it is within each
JSON construct.

Updating the variables included is relatively easy, because storing and retrieval is
done by datestamp key and involves a replacement of all the data in that cell.

The `daywiz.py` is careful to map available data to the up-to-date data structure
expected. Missing data is therefore handled gracefully.

When the data structure is updated in `daywiz.py` the new data structure will be used
and stored when new data is saved. Hence, old data formats will load without causing
and error and new formats are adopted smoothly. (See `get_day_data()`,
`get_wiz_data()` and `daypage_wiztable.merge_data()` in `daywiz.py`.)

Therefore, to update the Wiztable data structure, just update `wiztable.py`.

## More

See https://trello.com/c/JssbodOF .

Also, note that the metrics defined and calculated in the Formalizer component
`tools/system/metrics` are separate from those in DayWiz.

--
Randal A. Koene, 2023
