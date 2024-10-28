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

function set_hover_delayed_function(func_ref, delay_ms) {
  // All elements on the page with the hoverdelayfunc class.
  const elements = document.querySelectorAll(".hoverdelayfunc");
  var prepared = 0;
  for (let i = 0; i < elements.length; i++) {
    hoverWithDelay(elements[i], func_ref, delay_ms);
    prepared++;
  }
  console.log(`Prepared ${prepared} delayed hover elements.`);
}

function openPopup(url) {
  const newWindow = window.open(url, '_blank', 'height=500,width=800');

  if (window.focus) {
    newWindow.focus();
  }
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
