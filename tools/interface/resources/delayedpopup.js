/*
To use this:
1. Include in body: <script type="text/javascript" src="/delayedpopup.js"></script>
2. After that, in body, something like:
   <script>
   set_hover_delayed_function('.docpopupfunc', openPopup, 1000);
   </script>
3. Add the popup function if it is not one already included in this JS file.
4. Give some content the requisite class, e.g.:
   <a href="somehere.html" class="docpopupfunc">some link</a> 
Note that the openHiddenPopup() function is specifically designed to work
with a <span> of that class that has a "data-value" attribute. While openPopup()
is specific to <a> tags and enlargeImage() is specific to <img> tags, this
can be used with almost any page content.
There, you can also provide a preferred popup width and height through
"data-width" and "data-height" attributes.

To use automated conversion of copied links into popup links, include the
following in the body:
<script>
pastePopupLink('docpopupfunc', 'entrytext');
</script>
Where 'docpopupfunc' refers to the class identifier to give to popup links
and 'entrytext' refers to the ID of the <textarea> element that should be
listening for paste actions.
See, for example, how this is included in the Edit page through fzgraphhtml-cgi.py.
*/
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

function openHiddenPopup(data_ref) {
  const viewportWidth = window.innerWidth;
  const viewportHeight = window.innerHeight;
  const url = data_ref.dataset.value;
  var width = data_ref.dataset.width;
  var height = data_ref.dataset.height;
  if (!width) {
    width = "750px";
  } else {
    if (width.slice(-1)=="%") {
      let width_px = Number(width.slice(0,-1))*viewportWidth/100;
      width = `${width_px}px`;
    }
  }
  if (!height) {
    height = "750px";
  } else {
    if (height.slice(-1)=="%") {
      let height_px = Number(height.slice(0,-1))*viewportHeight/100;
      height = `${height_px}px`;
    }
  }
  console.log(`Using width=${width} and height=${height} for popup.`);
  const newDiv = document.createElement("div");
  newDiv.innerHTML = `<iframe src="${url}" width="${width}" height="${height}"></iframe>`;
  document.body.appendChild(newDiv);
  newDiv.style.zIndex = "999";
  newDiv.style.position = "fixed";
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
      pastedText = `<a href="${pastedText}" class="${class_id}" target="_blank">${pastedText}</a>`;
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
