'use strict';

// HTML elements

let indexFileElement;
let refFileElement;
let targetListFileElement;
let maxNumMismatchesInTargetElement;
let backgroundModeElement;
let backgroundListFileLabelElement;
let backgroundListFileElement;
let maxNumMismatchesInBackgroundElement;
let includeLoopPrimersElement;
let numPrimersToGenerateElement;
let toggleLogBtn;
let logElement;
let runBtn;
let resultsElement;

let parametersPageElement;
let progressPageElement;

// Input validation

function validateFastaFileInput(inputElement) {
    const isValid = inputElement.files.length == 1;

    const labelElement = getLabelForInputElement(inputElement);
    if (labelElement) {
        if (isValid)
            labelElement.classList.remove('invalidInput');
        else
            labelElement.classList.add('invalidInput');
    }

    return isValid;
}

// Returns true if the inputs are valid, false otherwise
function validateInputs() {
    let invalidCount = 0;

    invalidCount += validateFastaFileInput(indexFileElement) ? 0 : 1;
    invalidCount += validateFastaFileInput(refFileElement) ? 0 : 1;
    invalidCount += validateFastaFileInput(targetListFileElement) ? 0 : 1;
    if (backgroundModeElement.value == 'fromFile') {
        invalidCount += validateFastaFileInput(backgroundListFileElement) ? 0 : 1;
    }

    return invalidCount == 0;
}

function getLabelForInputElement(inputElement) {
    var element = inputElement;
    while (element) {
        if (element.nodeName == 'LABEL') {
            return element;
        }

        element = element.parentNode;
    }
    return null;
}

// Progress infos

function clearProgressDetails() {
    for (const el of document.getElementsByClassName('progressDetails'))
        el.innerText = '';
}

function setActivePhase(phase) {
    const phaseElements = [
        document.getElementById('buildBowtieIndex'),
        document.getElementById('generateSingleRegionPrimers'),
        document.getElementById('alignSingleRegionPrimers'),
        document.getElementById('generateLampPrimerSets'),
    ];

    var found = false;
    for (const element of phaseElements) {
        const progressIconElement = element.getElementsByClassName('progressIcon')[0];

        if (element.id == phase) {
            progressIconElement.classList.add('activePhase')
            progressIconElement.innerText = '⏳';
            found = true;
        } else {
            progressIconElement.classList.remove('activePhase')
            progressIconElement.innerText = '...';
            if (!found) {
                progressIconElement.innerText = '✅';
            }
        }
    }
}

function toggleLog() {
    if (logElement.style.display === 'none') {
        toggleLogBtn.innerText = 'Hide';
        logElement.style.display = '';
    } else {
        toggleLogBtn.innerText = 'Show';
        logElement.style.display = 'none';
    }
}

function updateBackgroundListFileLabelVisibility() {
    if (backgroundModeElement.value === 'fromFile') {
        backgroundListFileLabelElement.style.display = '';
    } else {
        backgroundListFileLabelElement.style.display = 'none';
    }
}

// Init

function initHTMLElements() {
    indexFileElement = document.getElementById('indexFile');
    refFileElement = document.getElementById('refFile');
    targetListFileElement = document.getElementById('targetListFile');
    maxNumMismatchesInTargetElement = document.getElementById('maxNumMismatchesInTarget');
    backgroundModeElement = document.getElementById('backgroundMode');
    backgroundListFileLabelElement = document.getElementById('backgroundListFileLabel');
    backgroundListFileElement = document.getElementById('backgroundListFile');
    maxNumMismatchesInBackgroundElement = document.getElementById('maxNumMismatchesInBackground');
    includeLoopPrimersElement = document.getElementById('includeLoopPrimers');
    numPrimersToGenerateElement = document.getElementById('numPrimersToGenerate');
    toggleLogBtn = document.getElementById('toggleLogBtn');
    logElement = document.getElementById('log');
    runBtn = document.getElementById('runBtn')
    resultsElement = document.getElementById('results');

    parametersPageElement = document.getElementById('parametersPage');
    progressPageElement = document.getElementById('progressPage');
}

function init() {
    initHTMLElements();
    updateBackgroundListFileLabelVisibility();

    const worker = new Worker('worker.js');

    worker.onmessage = (e) => {
        const msg = e.data;
        if (!('cmd' in msg)) {
            console.log('Bad message');
            return;
        }

        const cmd = msg.cmd;
        if (cmd == 'print' || cmd == 'printErr') {
            logElement.value += msg.text + "\n";
            logElement.scrollTop = logElement.scrollHeight;
        } else if (cmd == 'results') {
            resultsElement.style.display = 'block';
            resultsElement.value = msg.results;
            const h2Element = progressPageElement.getElementsByTagName('h2')[0];
            if (h2Element)
                h2Element.innerText = 'Done';
            setActivePhase('');
            clearProgressDetails();
            window.removeEventListener("beforeunload", beforeUnloadHandler);
        } else if (cmd == 'notify_about_to_start_phase') {
            const { phase } = msg;
            setActivePhase(phase);
            clearProgressDetails();
        } else if (cmd == 'notify_about_to_check_candidate_primer_region') {
            const { current, total } = msg;
            const percentage = current / total * 100;
            const progressElement = document.getElementById('generateSingleRegionPrimersProgress');
            progressElement.innerText = `${current}/${total} (${percentage.toFixed(0)}%)`;
        } else if (cmd == 'notify_about_to_check_primer_set_candidate') {
            const { numTargets, current, total } = msg;
            const percentage = current / total * 100;
            const progressElement = document.getElementById('generateLampPrimerSetsProgress');
            const genomes = numTargets > 1 ? 'genomes' : 'genome';
            progressElement.innerText = `Amplify ${numTargets} ${genomes} ${current}/${total} (${percentage.toFixed(0)}%)`;
        } else if (cmd == 'notify_found_primer_set_candidate') {
            const { f3, f2, f1c, b1c, b2, b3, lf, lb, } = msg.args;
            console.log('Found LAMP primer set', f3, f2, f1c, b1c, b2, b3, lf, lb);
        } else {
            console.log(`Unknown command: ${cmd}`);
        }
    };

    for (const fastaFileInput of [indexFileElement, refFileElement, targetListFileElement, backgroundListFileElement]) {
        fastaFileInput.addEventListener('change', () => validateFastaFileInput(fastaFileInput));
    }

    async function runGlapd() {
        if (!validateInputs())
            return;

        const args = {
            index: await indexFileElement.files[0].text(),
            ref: await refFileElement.files[0].text(),
            targetList: await targetListFileElement.files[0].text(),
            maxNumMismatchesInTarget: maxNumMismatchesInTargetElement.value,
            backgroundMode: backgroundModeElement.value,
            backgroundList: backgroundModeElement.value == 'fromFile' ? await backgroundListFileElement.files[0].text() : null,
            maxNumMismatchesInBackground: maxNumMismatchesInBackgroundElement.value,
            includeLoopPrimers: includeLoopPrimersElement.checked,
            numPrimersToGenerate: numPrimersToGenerateElement.value,
        };

        // Dispatch to worker
        worker.postMessage(args);

        // Update HTML
        window.addEventListener("beforeunload", beforeUnloadHandler);
        runBtn.disabled = true;
        parametersPageElement.style.display = 'none';
        progressPageElement.style.display = 'block';
        logElement.value = '';
        resultsElement.value = '';
    }

    runBtn.addEventListener('click', e => runGlapd());

    toggleLogBtn.addEventListener('click', () => toggleLog());

    backgroundModeElement.addEventListener('change', () => updateBackgroundListFileLabelVisibility());
}

if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
} else {
    init();
}

// Register with window's beforeunload event to ask user for confirmation before closing
function beforeUnloadHandler(event) {
    event.preventDefault();
    event.returnValue = true;
};