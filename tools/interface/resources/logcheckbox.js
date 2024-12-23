function findNearestLinkBefore(element) {
  let current = element.previousElementSibling;

  while (current) {
    if (current.tagName === 'A') {
      return current;
    }
    current = current.previousElementSibling;
  }

  return null; // No link found
}

// Get the table element
const table = document.getElementById("LogInterval");

// Add an event listener to the table
table.addEventListener("change", (event) => {
  // Check if the target is a checkbox
  if (event.target.type === "checkbox") {
    // Do something with the checkbox
    console.log("Checkbox changed:", event.target.checked);
    const targetElement = event.target;
    var precedinglink = findNearestLinkBefore(targetElement)
    console.log(precedinglink.url);
  }
});
