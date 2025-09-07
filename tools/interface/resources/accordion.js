/*
  This script will listen for clicks on the question headers and toggle the "active" state.

  Include accordion.css in the HTML file. The HTML file should be something like this:

    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Collapsible Questions</title>
        <link rel="stylesheet" href="style.css">
    </head>
    <body>

        <div class="accordion">

            <button class="accordion-header">
                Question 1: What is HTML?
            </button>
            <div class="accordion-content">
                <p>HTML stands for HyperText Markup Language. It is the standard markup language for documents designed to be displayed in a web browser.</p>
            </div>

            <button class="accordion-header">
                Question 2: What is CSS?
            </button>
            <div class="accordion-content">
                <p>CSS stands for Cascading Style Sheets. It is used for describing the presentation of a document written in a markup language like HTML.</p>
            </div>

            <button class="accordion-header">
                Question 3: What is JavaScript?
            </button>
            <div class="accordion-content">
                <p>JavaScript is a programming language that is one of the core technologies of the World Wide Web. It enables interactive web pages.</p>
            </div>

        </div>

        <script src="script.js"></script>

    </body>
    </html>
*/

document.addEventListener('DOMContentLoaded', function() {
    const headers = document.querySelectorAll('.accordion-header');

    headers.forEach(header => {
        const content = header.nextElementSibling;

        // Event listener to open the content when the header is hovered over
        header.addEventListener('mouseover', function() {
            // Expand the content by setting its maxHeight to its scrollHeight
            content.style.maxHeight = content.scrollHeight + "px";
            content.style.padding = "10px 18px";
        });
        
        // Event listener to close the content when the header is clicked
        header.addEventListener('click', function(event) {
            // Prevent the default hover behavior from re-opening it immediately
            event.stopPropagation();
            
            // Collapse the content by setting its maxHeight to null
            content.style.maxHeight = null;
            content.style.padding = "0 18px";
        });

        // Optional: Add a mouseout event to collapse if the user hovers away
        // without clicking. This can make the experience more intuitive.
        // header.addEventListener('mouseout', function() {
        //     // Check if the content is not already closed by a click
        //     if (content.style.maxHeight !== "0px" && content.style.maxHeight !== null) {
        //         content.style.maxHeight = null;
        //         content.style.padding = "0 18px";
        //     }
        // });
    });
});
