/*
  Retrieve encrypted data in JSON format from database table,
  decrypt and send to display function.

  Randal A. Koene, 20241027
*/

var retrieved_str = '';
var decrypted_str = '';
var cached_passwordKey = null;

async function getJSONfromDatabase(url, index) {
    fetch(url)
    .then(response => response.json())
    .then((data) => {
        retrieved_str = data[index];
    });
}

async function openPasswordModal(pwblock_id) {
    document.getElementById(pwblock_id).style.display = 'block';
}

// for large strings, use this from https://stackoverflow.com/a/49124600
const buff_to_base64 = (buff) => btoa(
  new Uint8Array(buff).reduce(
    (data, byte) => data + String.fromCharCode(byte), ''
  )
);

const base64_to_buf = (b64) =>
  Uint8Array.from(atob(b64), (c) => c.charCodeAt(null));

const enc = new TextEncoder();
const dec = new TextDecoder();

const getPasswordKey = (password) =>
  window.crypto.subtle.importKey("raw", enc.encode(password), "PBKDF2", false, [
    "deriveKey",
  ]);

const deriveKey = (passwordKey, salt, keyUsage) =>
  window.crypto.subtle.deriveKey(
    {
      name: "PBKDF2",
      salt: salt,
      iterations: 250, //250000,
      hash: "SHA-256",
    },
    passwordKey,
    { name: "AES-GCM", length: 256 },
    false,
    keyUsage
  );

async function decryptData(encryptedData, password) {
  try {
    const encryptedDataBuff = base64_to_buf(encryptedData);
    const salt = encryptedDataBuff.slice(0, 16);
    const iv = encryptedDataBuff.slice(16, 16 + 12);
    const data = encryptedDataBuff.slice(16 + 12);
    const passwordKey = await getPasswordKey(password);
    cached_passwordKey = passwordKey;
    const aesKey = await deriveKey(passwordKey, salt, ["decrypt"]);
    const decryptedContent = await window.crypto.subtle.decrypt(
      {
        name: "AES-GCM",
        iv: iv,
      },
      aesKey,
      data
    );
    return dec.decode(decryptedContent);
  } catch (e) {
    console.log('A decryption error occurred.');
    console.log(`Error - ${e}`);
    return "";
  }
}

async function decrypt_retrieved(password, displayJSONstr_func) {
    decrypted_str = await decryptData(retrieved_str, password);
    await displayJSONstr_func(decrypted_str);
}

function requestDecryptPassword(pwblock_id, pwsubmit_id, pwinput_id, displayJSONstr_func) {
    return new Promise(function (resolve, reject) {
        document.getElementById(pwsubmit_id).addEventListener('click', function() {
            // Handle password submission
            var password = document.getElementById(pwinput_id).value;
            console.log("Password entered:", password);
            document.getElementById(pwblock_id).style.display = 'none';
            decrypt_retrieved(password, displayJSONstr_func);
            resolve();
        });
    });
}

async function encryptData(secretData) { //, password) {
  try {
    const salt = window.crypto.getRandomValues(new Uint8Array(16));
    const iv = window.crypto.getRandomValues(new Uint8Array(12));
    //const passwordKey = await getPasswordKey(password);
    const passwordKey = cached_passwordKey;
    const aesKey = await deriveKey(passwordKey, salt, ["encrypt"]);
    const encryptedContent = await window.crypto.subtle.encrypt(
      {
        name: "AES-GCM",
        iv: iv,
      },
      aesKey,
      enc.encode(secretData)
    );

    const encryptedContentArr = new Uint8Array(encryptedContent);
    let buff = new Uint8Array(
      salt.byteLength + iv.byteLength + encryptedContentArr.byteLength
    );
    buff.set(salt, 0);
    buff.set(iv, salt.byteLength);
    buff.set(encryptedContentArr, salt.byteLength + iv.byteLength);
    const base64Buff = buff_to_base64(buff);
    return base64Buff;
  } catch (e) {
    console.log(`Error - ${e}`);
    return "";
  }
}

function sendEncryptedJSONtoDatabase(encrypted_str, url, tablename, index) {
  // Send encrypted data to database writing script
  console.log("Sending to database.");
  let formData = new FormData();
  formData.append('action', 'store');
  formData.append('tablename', tablename);
  formData.append('index', index); // *** Looks like these replace url arguments previously used.
  formData.append('data', encrypted_str);
  formData.append('type', 'text');
  fetch("/cgi-bin/fzmetricspq-cgi.py", { // *** How can this work? The url parameter is never propagated.
  method: 'POST',
  body: formData
  })
  .then(response => response.text())
  .then(data => {
    console.log(JSON.stringify(data));
  });
}

async function encryptJSONtoDatabase(unencrypted_str, url, tablename, index) {
  var encrypted_str = await encryptData(unencrypted_str, password);
  sendEncryptedJSONtoDatabase(encrypted_str, url, tablename, index);
}

export { getJSONfromDatabase, openPasswordModal, requestDecryptPassword, encryptJSONtoDatabase };
