function copyInnerHTMLToClipboard(element_id) {
  var copyValue = document.getElementById(element_id);
  var value_content = '---';
  if (copyValue == null) {
    console.log(`Did not find object with id ${element_id}.`);
  } else {
    value_content = copyValue.innerHTML;
    console.log(`Value of ${element_id} object is ${value_content}.`);
  }
  const tempTextArea = document.createElement("textarea");
  tempTextArea.value = value_content;
  document.body.appendChild(tempTextArea);
  tempTextArea.select();
  document.execCommand("copy");
  document.body.removeChild(tempTextArea);
  //navigator.clipboard.writeText(value_content);
  alert("Copied: " + value_content);
}