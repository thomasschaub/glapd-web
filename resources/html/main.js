function init() {
    const indexFileElement = document.getElementById('indexFile');
    const refFileElement = document.getElementById('refFile');
    const targetListFileElement = document.getElementById('targetListFile');
    const maxNumMismatchesInTargetElement = document.getElementById('maxNumMismatchesInTarget');
    const backgroundModeElement = document.getElementById('backgroundMode');
    const backgroundListFileElement = document.getElementById('backgroundListFile');
    const maxNumMismatchesInBackgroundElement = document.getElementById('maxNumMismatchesInBackground');
    const includeLoopPrimersElement = document.getElementById('includeLoopPrimers');
    const numPrimersToGenerateElement = document.getElementById('numPrimersToGenerate');
    const outputElement = document.getElementById('output');
    const runBtn = document.getElementById('runBtn')
    const resultsElement = document.getElementById('results');
    const progressInfoRootElement = document.getElementById('progressInfoRoot');
    const phaseElement = document.getElementById('phase');
    const progressElement = document.getElementById('progress');

    const worker = new Worker('worker.js');

    worker.onmessage = (e) => {
        const msg = e.data;
        if (!('cmd' in msg)) {
            console.log('Bad message');
            return;
        }

        const cmd = msg.cmd;
        if (cmd == 'print' || cmd == 'printErr') {
            outputElement.textContent += msg.text + "\n";
            outputElement.scrollTop = outputElement.scrollHeight;
        } else if (cmd == 'results') {
            phaseElement.innerText = 'Done';
            progressElement.innerText = '';
            resultsElement.style.display = 'block';
            resultsElement.textContent = msg.results;
        } else if (cmd == 'notify_about_to_start_phase') {
            const { phase } = msg;
            phaseElement.innerText = phase;
            progressElement.innerText = '';
        } else if (cmd == 'notify_about_to_check_candidate_primer_region') {
            const { current, total } = msg;
            progressElement.innerText = (current / total * 100).toFixed(2) + "%";
        } else if (cmd == 'notify_about_to_check_primer_set_candidate') {
            const { numTargets, current, total } = msg;
            progressElement.innerText = (current / total * 100).toFixed(2) + "%";
        } else {
            console.log(`Unknown command: ${cmd}`);
        }
    };

    async function runGlapd() {
        // Validate inputs
        if (indexFileElement.files.length != 1) {
            console.log('Index file not set');
            return;
        }
        if (refFileElement.files.length != 1) {
            console.log('Ref file not set');
            return;
        }
        if (targetListFileElement.files.length != 1) {
            console.log('Target list file not set');
            return;
        }
        if (backgroundModeElement.value == 'fromFile') {
            if (backgroundListFileElement.files.length != 1) {
                console.log('Background list file not set')
                return;
            }
        }

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
        runBtn.disabled = true;
        phaseElement.innerText = ''
        progressElement.innerText = ''
        outputElement.textContent = null;
        progressInfoRootElement.style.display = 'block';
    }

    document.getElementById('runBtn').addEventListener('click', e => runGlapd());
}

if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
} else {
    init();
}