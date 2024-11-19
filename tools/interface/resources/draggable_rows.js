// Make table rows draggable
const table = document.getElementById("nodedata");
const rows = table.querySelectorAll("tbody tr");

function getFromDateStamp(datestampstr) {
  const yearstr = datestampstr.substring(0,4);
  const monthstr = datestampstr.substring(4,6);
  const daystr = datestampstr.substring(6,8);
  const hourstr = datestampstr.substring(8,10);
  const minstr = datestampstr.substring(10,12);
  const dateString = `${yearstr}-${monthstr}-${daystr}T${hourstr}:${minstr}:00`; 
  return new Date(dateString);
}

function makeDateStamp(date_obj) {
  const stampFormat = `${date_obj.getFullYear()}${(date_obj.getMonth() + 1).toString().padStart(2, '0')}${date_obj.getDate().toString().padStart(2, '0')}${date_obj.getHours().toString().padStart(2, '0')}${date_obj.getMinutes().toString().padStart(2, '0')}`;
  return stampFormat;
}

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

  // Move row
  const draggedRowId = event.dataTransfer.getData("text/plain");
  const draggedRow = document.getElementById(draggedRowId);
  const targetRow = event.target.closest("tr");

  if (targetRow && targetRow !== draggedRow) {
    table.tBodies[0].insertBefore(draggedRow, targetRow.nextSibling);
  }

  // Mark moved row target date
  const tdcell = draggedRow.cells[3];
  tdcell.classList.add("high_req");
  draggedRow.dataset.modifiedTd = true;

  // Modify moved row target date
  const nextRow = draggedRow.nextElementSibling;
  if (!nextRow) {
    alert('No next row to obtain reference target date from.');
    return;
  }
  const nexttdcell = nextRow.cells[3];
  var td = getFromDateStamp(nexttdcell.textContent);
  td.setMinutes(td.getMinutes() - 1);
  var new_tdstamp = makeDateStamp(td);
  tdcell.textContent = `${new_tdstamp}`;

  // Check preceding target dates
  var currentRow = draggedRow;
  while (true) {
    const prevRow = currentRow.previousElementSibling;
    if (!prevRow) {
      break;
    }
    const prevtdcell = prevRow.cells[3];
    var prevtd = getFromDateStamp(prevtdcell.textContent);
    if (prevtd < td) {
      break;
    }
    prevtd.setMinutes(td.getMinutes() -1);
    var new_prevtdstamp = makeDateStamp(prevtd);
    prevtdcell.textContent = `${new_prevtdstamp}`;
    prevtdcell.classList.add("high_req");
    prevRow.dataset.modifiedTd = true;

    currentRow = prevRow;
    td = prevtd;
  }

});

function apply_td_updates() {
  alert('Confirm update?');
  var modified_data = [];
  var rowref = table.rows[0];
  while (rowref) {
    if ('modifiedTd' in rowref.dataset) {
      modified_data.push([ rowref.cells[3].textContent, rowref.id ]);
    }
    rowref = rowref.nextElementSibling;
  }
  console.log(`Number of Nodes to update: ${modified_data.length}`);
  console.log(modified_data);
}
