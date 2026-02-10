/*
  Call a Formalizer CGI utility and return response data.

  Randal A. Koene, 20260104
*/

// fzcgiutil is a Formalizer utility CGI script such as '/cgi-bin/fzloghtml-cgi.py'
// argstuples is something like [['-q',''],['-o','STDOUT'],[...]]
async function fzCGIRequest(fzcgiutil, argstuples) {
    let formData = new FormData();
    for (let [key, value] of argstuples) {
        formData.append(key, value);
    }

    try {
        const response = await fetch(fzcgiutil, {
            method: 'POST',
            body: formData
        });
        return await response.text();
    } catch (error) {
        console.error("Request Failed:", error);
        throw error;
    }
}

// fzcgiutil is a Formalizer utility CGI script such as '/cgi-bin/fzloghtml-cgi.py'
// argstuples is something like [['-q',''],['-o','STDOUT'],[...]]
// elementid is the ID of some element in the HTML page
// handler is a response handler function
function setupfzCGIListener(elementid, fzcgiutil, argstuples, handler) {
    const element = document.getElementById(elementid);
    
    element.addEventListener('click', async () => {
        //console.log(`Requesting ${fzcgiutil}...`);
        
        // Wait for the actual network data
        const data = await fzCGIRequest(fzcgiutil, argstuples);
        
        // Handle the data
        handler(data);
        
        // Now this will log the updated data
        //console.log('Retrieving updated dataset...');
        //console.log(element.dataset.response);
    });
}

// fzcgiutil is a Formalizer utility CGI script such as '/cgi-bin/fzloghtml-cgi.py'
// argstuples is something like [['-q',''],['-o','STDOUT'],[...]]
// handler is a response handler function
async function pollfzServer(fzcgiutil, argstuples, handler, poll_ms) {
    const data = await fzCGIRequest(fzcgiutil, argstuples); // Wait for the network
    handler(data);
    
    // Only schedule the NEXT call after this one is finished
    setTimeout(pollfzServer, poll_ms, fzcgiutil, argstuples, handler, poll_ms);
}

// Where you use this module, add something like:
//
// setupfzCGIListener('some_id', fzcgiutil, argstuples, responseHandler);
// pollServer(fzcgiutil, argstuples, responseHandler); // Start the loop

function getResponseJson(responseString) {
    try {
        const data = JSON.parse(responseString);
        return data;
    } catch (error) {
        // If an error is caught, the string is not valid JSON
        return {};
    }
}

async function fzAPICall(apicall) {
    const fzcgiutil = '/cgi-bin/fzapicall.py';
    const argstuples = [['apicall', apicall]];
    const response = await fzCGIRequest(fzcgiutil, argstuples);
    //console.log(response);
    return getResponseJson(response);
}

export { fzCGIRequest, setupfzCGIListener, pollfzServer, getResponseJson, fzAPICall };
