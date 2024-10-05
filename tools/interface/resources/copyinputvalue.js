function copyInputValueToClipboard(element_id) {
  var copyValue = document.getElementById(element_id);
  var value_content = '---';
  if (copyValue == null) {
    console.log(`Did not find object with id ${element_id}.`);
  } else {
    value_content = copyValue.value;
    console.log(`Value of ${element_id} object is ${value_content}.`);
  }
  copyValue.select();
  //copyValue.setSelectionRange(0, 99999); // For mobile devices
  navigator.clipboard.writeText(value_content);
  alert("Copied: " + value_content);
}
