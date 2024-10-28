function hoverWithDelay(element, callback, delay) {
  let timeoutId;

  element.addEventListener('mouseover', () => {
    timeoutId = setTimeout(() => {
      callback(element);
    }, delay);
  });

  element.addEventListener('mouseout', () => {
    clearTimeout(timeoutId);
  });
}

function set_hover_delayed_function(class_dotid, func_ref, delay_ms) {
  // All elements on the page with the hoverdelayfunc class.
  const elements = document.querySelectorAll(class_dotid);
  var prepared = 0;
  for (let i = 0; i < elements.length; i++) {
    hoverWithDelay(elements[i], func_ref, delay_ms);
    prepared++;
  }
  console.log(`Prepared ${prepared} delayed hover elements.`);
}

function openPopup(a_ref) {
  const url = a_ref.href;
  const newDiv = document.createElement("div");
  newDiv.innerHTML = `<iframe src="${url}" width = "750px" height = "750px"></iframe>`;
  document.body.appendChild(newDiv);
  newDiv.style.zIndex = "999";
  newDiv.style.position = "fixed";
  const viewportWidth = window.innerWidth;
  const viewportHeight = window.innerHeight;
  const elementWidth = newDiv.offsetWidth;
  const elementHeight = newDiv.offsetHeight;
  newDiv.style.left = (viewportWidth - elementWidth) / 2 + "px";
  newDiv.style.top = (viewportHeight - elementHeight) / 2 + "px";
  newDiv.addEventListener('mouseout', () => {
    newDiv.remove();
  });
  console.log("Preview generated.");
}

function enlargeImage(img_ref) {
  const newImg = document.createElement("img");

  newImg.src = img_ref.src;
  newImg.style.border = '1px solid black';
  document.body.appendChild(newImg);

  newImg.style.zIndex = "999";
  newImg.style.position = "fixed";

  const viewportWidth = window.innerWidth;
  const viewportHeight = window.innerHeight;
  const elementWidth = newImg.offsetWidth;
  const elementHeight = newImg.offsetHeight;
  newImg.style.left = (viewportWidth - elementWidth) / 2 + "px";
  newImg.style.top = (viewportHeight - elementHeight) / 2 + "px";

  newImg.addEventListener('mouseout', () => {
    newImg.remove();
  });
}

function pastePopupLink(class_id, textarea_id) {
  const textarea = document.getElementById(textarea_id);

  textarea.addEventListener("paste", (event) => {

    // Prevent the default paste behavior
    event.preventDefault();

    // Get the pasted text from the clipboard
    var pastedText = event.clipboardData.getData('text/plain');

    const http_regex = /^http:/;
    const https_regex = /^https:/;

    if (pastedText.match(http_regex) || pastedText.match(https_regex)) {
      pastedText = `<a class="${class_id}" href="${pastedText}" target="_blank">${pastedText}</a>`;
    }

    // Get the current cursor position
    const start = textarea.selectionStart;
    const end = textarea.selectionEnd;

    // Insert the pasted text at the cursor position
    const originalText = textarea.value;
    textarea.value = originalText.slice(0, start) + pastedText + originalText.slice(end);

    // Update the cursor position after pasting
    textarea.setSelectionRange(start + pastedText.length, start + pastedText.length);

  });
}
