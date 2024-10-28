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
