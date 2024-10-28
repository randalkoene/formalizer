/*
  Retrieve encrypted data in JSON format from database table,
  decrypt and send to display function.

  Randal A. Koene, 20241027
*/

var retrieved_str = '';
var decrypted_str = '';

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
    displayJSONstr_func(decrypted_str);
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

export { getJSONfromDatabase, openPasswordModal, requestDecryptPassword };
