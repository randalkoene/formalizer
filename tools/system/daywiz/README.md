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
7. `calendar-info.py`: Pull Google Calendar data.
8. `convert_to_database.py`: Conversion from JSON data file to Formalizer database table.
9. `daydata_from_database.py`: Testing retrieval of a day's data from the Formalizer database table.
10. `daywiz-autodata-cgi.py`: CGI entry point to retrieve number of emails, numbe of checkboxes, upcoming calendar entries, and 24 hour emails received
11. `daywiz-autodata.py`: Retrieve number of emails, numbe of checkboxes, upcoming calendar entries, and 24 hour emails received.
12. `email-info-cgi.py`: CGI entry point to run `email-info.py`.
13. `email-info.py`: Retrieve number of emails unread and optionally the last 24 hours of emails received.
14. `inspect_data.py`: Inspect the data format and more for data stored in the Formalizer database table.

Note that `daywiz_json.py` is like `daywiz.py`, but works with the JSON data source
and is now deprecated.

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
an error and new formats are adopted smoothly. (See `get_day_data()`,
`get_wiz_data()` and `daypage_wiztable.merge_data()` in `daywiz.py`.)

Therefore, to update the Wiztable data structure, just update `wiztable.py`.

## More

See https://trello.com/c/JssbodOF .

Also, note that the metrics defined and calculated in the Formalizer component
`tools/system/metrics` are separate from those in DayWiz.

--
Randal A. Koene, 2023
