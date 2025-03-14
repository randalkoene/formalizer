/**
 * score.js, Randal A. Koene 2024
 *
 * Floats a score in the lower left hand corner of the browser view port.
 *
 * To include this on a page:
 * - Load this before the closing </body> tag: <script type="text/javascript" src="/score.js">v</script>
 * - Load the necessary CSS in the <head> section (e.g. before <title>): <link rel="stylesheet" href="/score.css">
 * - Add a button to associate the score with, somewhere in the body: <button id="score" class="button button2">_____</button>
 *
 * Adapted from the clock.js script.
 */

const SCORE_TOOLTIP='<span class="alt_tooltiptext" style="bottom: 100%;"><div style="width: 300;">The score shown is the rounded 10x DayWiz score / max. score possible for the last 7 days. A 10 would be a perfect score.</div></span>';

class floatScore {
    /**
     * @param score_id The DOM `id` of the element that displays the score.
     * @param autostart If true then call `this.scoreStart()` in object constructor.
     */
    constructor(score_id = 'score', autostart = true) {
        this.intervaltask = null;
        this.scoreContent = '??';
        this.scoreelement = this.getScoreElement(score_id);
        console.log('floatScore instance created');
        if (autostart) {
            this.scoreStart();
        }
    }

    getScoreElement(score_id) {
        var scorebtnelement = document.getElementById(score_id);
        if (scorebtnelement == null) {
            console.log('Making score element');
            scorebtnelement = this.makeScoreElement(score_id);
        }
        return scorebtnelement;
    }

    makeScoreElement(score_id) {
        var scorebtnelement = document.createElement("div");
        scorebtnelement.innerHTML = `<button id="${score_id}" class="button button2 alt_tooltip" onclick="window.open('/cgi-bin/score.py?cmd=show&selectors=wiztable', '_blank');">_____</button>`;
        document.body.prepend(scorebtnelement);
        return document.getElementById(score_id);
    }

    makeScoreString(_content ) {
        return `${_content}${SCORE_TOOLTIP}`;
    }

    updateScore() {
        this.readScores();
        var _content = '';
        if (window.scores_json) {
            for (const [key, value] of Object.entries(window.scores_json)) {
                //console.log(`Score ${key} and ${value}.`);
                _content += ` ${value}`;
            }
        } else {
            //console.error('No scores available.')
        }
        this.scoreelement.innerHTML = this.makeScoreString(_content);
    }

    scoreStart() {
        if (this.intervaltask != null) {
            console.error('Unable to start clock when there is already an active clock interval task running.');
            return;
        }
        this.updateScore();
        this.intervaltask = setInterval(this.updateScore.bind(this), 10000); // every ten seconds
        console.log('floatScore started');
    }

    readScores() {
        var scores = new XMLHttpRequest();

        scores.onreadystatechange = function() {
            if (this.readyState == 4) {
                if (this.status == 200) {
                    var scores_data = scores.responseText;
                    //console.log(`Score loaded: ${scores_data}`);
                    window.scores_json = JSON.parse(scores_data);
                } else {
                    console.error('Scores loading failed.');
                    window.scores_json = {}
                }
            }
        };

        scores.open( 'GET', '/formalizer/data/daywiz_total_scores.json' );
        scores.send(); // no form data presently means we're requesting score
    }

};

window.global_score = new floatScore('score');
