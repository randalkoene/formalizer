// Make table rows draggable
const table = document.getElementById("nodedata");
const rows = table.querySelectorAll("tbody tr");

rows.forEach((row) => {
  row.draggable = true;

  row.addEventListener("dragstart", (event) => {
    event.dataTransfer.setData("text/plain", row.id);
  });
});

// Handle drop events
table.addEventListener("dragover", (event) => {
  event.preventDefault();
});

table.addEventListener("drop", (event) => {
  event.preventDefault();

  const draggedRowId = event.dataTransfer.getData("text/plain");
  const draggedRow = document.getElementById(draggedRowId);
  const targetRow = event.target.closest("tr");

  if (targetRow && targetRow !== draggedRow) {
    table.tBodies[0].insertBefore(draggedRow, targetRow.nextSibling);
  }
});
