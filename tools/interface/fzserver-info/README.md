# fzserver-info - Inspect the state of memory-resident servers.

For example, you may want to check the memory-resident existence and state of Graph data.

This tool can provide such information on the command line. It can also work with a small
Python CGI script to deliver the information in a browser-visual form.

## Server IP address

You can obtain the server IP address from the server status information
in a number of different formats. For example:

  `fzserver-info -G -F json`

The call above returns a JSON dictionary, in which the element
"listen_location" specifies the IP address and port of the running
fzserverpq instance.

Note that there is now also a direct TCP call to a running fzserverpq
instance that can obtain the IP address and port. This is useful for
core and tool applications that need that to craft URLs.

Those programs can use at least two ways to obtain the IP:Port string:

```
std::string get_server_address() {
    VERBOSEOUT("Sending API request to Server port.\n");
    std::string response_str;
    if (!http_GET(graph().get_server_IPaddr(), graph().get_server_port(), "/fz/ipport", response_str)) {
        return standard_exit_error(exit_communication_error, "API request to Server port failed ", __func__);
    }

    auto response_vec = split(response_str,'\n');
    for (const auto & line_str : response_vec) {
        if (line_str.substr(0,15) == "Server address:") {
            std::string ipport = line_str.substr(16);
            VERYVERBOSEOUT("Server address:\n\n"+ipport);
            return ipport;
        }
    }
    VERBOSEOUT("No server address in response received.")
    return ""
}
```

Or, via the library call `Graph::get_server_full_address()`.

See, for example, how this is done in fzdashboard.


--
Randal A. Koene, 2020, 2023
