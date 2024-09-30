const button_ids_links = [
    [ 'duplicate', '/index.html', '2' ],
    [ 'classical', 'https://www.youtube.com/playlist?list=PLL1hpR1qBIPxdxF1BhIPpl6s08kllCLs5', '3' ],
    [ 'EBM', 'https://www.youtube.com/playlist?list=PLL1hpR1qBIPzyg3_daMmuG79hZ56uHeCy', '3' ],
    [ 'industrial', 'https://www.youtube.com/playlist?list=PLL1hpR1qBIPwXtRPtB6dyUFg3CC0Fbreo', '3' ],
    [ 'meditative', 'https://www.youtube.com/playlist?list=PLL1hpR1qBIPypoHjDibsdYpTg606q8V1F', '3' ],
];

function makeRightBarElements(rightbar_id) {
    var rightbarbtnelement = document.createElement("div");
    var buttons_html = '';
    for (let i = 0; i < button_ids_links.length; i++) {
        buttons_html += `<button id="${button_ids_links[i][0]}" class="button button${button_ids_links[i][2]}" onclick="window.open('${button_ids_links[i][1]}','_blank');">${button_ids_links[i][0]}</button>\n`;
    }
    rightbarbtnelement.innerHTML = buttons_html;
    document.body.prepend(rightbarbtnelement);
    return document.getElementById(rightbar_id);
}

makeRightBarElements('rightbar');
