# fzloghtml - Generate HTML representation of requested Log records.

## To create a special URL that tells rendering to make a link to local files

In the URL part of a HTML link, put something like:

@FZSERVER@/doc/tex/something.doc

## The `day review` option

The `GraphTypes.hpp:Boolean_Tag_Flags` are used to specify Node category.
Currently, those are (aside from `tzadjust` and `error`):
`work`, `self_work`, `system`, `other`, or `none`.

This is carried out as followed in the `render.cpp:render_Log_review()` function:

1. Retrieve the NNL that lists all sleep Nodes.
2. Identify which of those is the nap Node.
3. Walk through completed Log chunks in the interval that was loaded.
4. Detect sleep chunks and use those to determine wake-up and go-to-sleep times.
5. Naps appear to receive the boolean tag flag `none`.
6. For other Nodes, detect the category and specify it in the boolean tag flag as detailed below.
7. (Also, collect metric tags data for tags such as @NMVIDSTART@ and @NMVIDSTOP@. If found, that is written to a CSV file.)
8. Render as json, html, txt or raw.

Detecting Node/Chunk category:

1. Search for category override tags in the Log entries of the chunk.
2. Get the Boolean Tag Flag with the function `Map_of_Subtrees::node_in_heads_or_any_subtree()`.

The `Map_of_Subtrees` was collected for the head Nodes and their dependencies as
specified in the `config.json` of `fzloghtml`. By default the NNL specified there
is `threads`.

Note that the categories search, as implemented in `fzloghtml`, does not search for
overrides in the Node description, nor does it search for category specification by
superior if not found for a Node. This means that many Nodes that are not covered
by the map of subtrees for the specified NNL receive the `none` Boolean_Tag_Flag.

## Examples of scripts that use this

- fzlog-cgi.py
- fztask-cgi.py
- fztask.py
- fzcatchup.py
- fzlink.py
- checkboxes.py
- fzlog-mostrecent.sh
- fzloghtml-cgi.py
- fzloghtml-term-1920.sh
- fzloghtml-term.sh
- get_log_entry.sh
- dayreview.py

## Prepared code that will probably be added to the Log page

The following is some Javascript code that will help to collect necessary information
to automate an 'edit' call that can update a clicked checkbox:

```
document.addEventListener('click', function(event) {
    if (event.target.type === 'checkbox') {
        let nearestLink = null;
        let textUntilNewline = '';
        let currentNode = event.target;
        
        // Part 1: Find nearest preceding link
        let linkSearchNode = event.target.previousSibling;
        while (linkSearchNode) {
            if (linkSearchNode.nodeType === Node.ELEMENT_NODE) {
                const links = linkSearchNode.querySelectorAll('a[href*="/cgi-bin/fzlog-cgi.py"]');
                if (links.length > 0) {
                    nearestLink = links[links.length - 1].href;
                    break;
                }
                
                if (linkSearchNode.tagName === 'A' && linkSearchNode.href.includes('/cgi-bin/fzlog-cgi.py')) {
                    nearestLink = linkSearchNode.href;
                    break;
                }
            }
            linkSearchNode = linkSearchNode.previousSibling;
            
            if (!linkSearchNode) {
                linkSearchNode = event.target.parentNode;
                event.target = linkSearchNode;
                if (linkSearchNode) {
                    linkSearchNode = linkSearchNode.previousSibling;
                }
            }
        }
        
        // Part 2: Capture text until next newline
        const treeWalker = document.createTreeWalker(
            document.body,
            NodeFilter.SHOW_TEXT,
            { acceptNode: function(node) { 
                return NodeFilter.FILTER_ACCEPT; 
            }},
            false
        );
        
        let foundCheckbox = false;
        while (treeWalker.nextNode()) {
            const textNode = treeWalker.currentNode;
            
            if (textNode.parentNode === currentNode || textNode.parentNode.contains(currentNode)) {
                foundCheckbox = true;
                continue;
            }
            
            if (foundCheckbox) {
                const textContent = textNode.nodeValue;
                const newlineIndex = textContent.indexOf('\n');
                
                if (newlineIndex >= 0) {
                    textUntilNewline += textContent.substring(0, newlineIndex);
                    break;
                } else {
                    textUntilNewline += textContent;
                }
            }
        }
        
        // Trim whitespace from the captured text
        textUntilNewline = textUntilNewline.trim();
        
        // Now you have both variables available:
        // nearestLink - contains the URL or null if not found
        // textUntilNewline - contains all text until the next newline after the checkbox
        
        console.log('Nearest link:', nearestLink);
        console.log('Text until newline:', textUntilNewline);
        
        // Example usage:
        // const targetLink = nearestLink;
        // const checkboxContextText = textUntilNewline;
    }
});
```

---
Randal A. Koene, 2020
