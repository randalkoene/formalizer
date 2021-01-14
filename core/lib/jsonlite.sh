# jsonlite.sh
# Bash library script
# Similar in utility purpose to jsonlite.cpp/hpp, but even lighter.
#
# Randal A. Koene, 20210113

# @param $1 String containing '"somelabel" : "somevalue"'.
function json_param_value() {

    # separate at colon
    json_param=${1%%:*}
    json_value="${1##*:}"

    # process only '"somelabel" : "somevalue"' lines
    if [ "$json_param" != "$1" ]; then

        # strip before first quotes and after last quotes
        json_param=${json_param#*\"}
        json_value="${json_value#*\"}"
        json_param=${json_param%\"*}
        json_value="${json_value%\"*}"

        # export as actual variable with value
        #declare -x "$json_param"="$json_value"
        export $json_param="$json_value"

        # uncomment for testing only
        #echo "Found variable $json_param with value $json_value"

    fi

}

# @param $1 Name of the JSON file.
function json_get_label_value_pairs_from_file() {

    while IFS= read -r line
    do
        json_param_value "$line"
    done < "$1"

}

# @param $1 String containing the lines of a JSON file.
function json_get_label_value_pairs_from_string() {

    while IFS= read -r line
    do
        json_param_value "$line"
    done <<< $(echo "$1")

}
