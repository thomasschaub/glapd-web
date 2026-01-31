var Module = {
    'noInitialRun': true,
};

Module.print = (text) => {
    postMessage({
        'cmd': 'print',
        'text': text,
    });
}

Module.printErr = (text) => {
    postMessage({
        'cmd': 'printErr',
        'text': text,
    });
}

Module.onRuntimeInitialized = () => {
    self.onmessage = (e) => {
        const msg = e.data;

        FS.mkdir('inputs')
        FS.writeFile('inputs/index.fa', msg.index);
        FS.writeFile('inputs/ref.fa',msg.ref);
        FS.writeFile('inputs/target.fa', msg.targetList);
        if (msg.backgroundMode == 'fromFile')
            FS.writeFile('inputs/background.fa', msg.backgroundList);

        const args = [
            '--index', 'inputs/index.fa',
            '--ref', 'inputs/ref.fa',
            '--target', 'inputs/target.fa',
            '--maxNumMismatchesInTarget', msg.maxNumMismatchesInTarget,
            '--backgroundMode', msg.backgroundMode,
            // --backgroundListPath is handled below
            '--maxNumMismatchesInBackground', msg.maxNumMismatchesInBackground,
            // --includeLoopPrimers is handled below
            '--numPrimersToGenerate', msg.numPrimersToGenerate,
            '--numThreads', '1', // TODO fix pthreads, then use: String(navigator.hardwareConcurrency),
        ];
        if (msg.backgroundMode == 'fromFile')
            args.push('--backgroundListPath', 'inputs/background.fa');
        if (msg.includeLoopPrimers)
            args.push('--includeLoopPrimers');

        const exitCode = callMain(args);

        const tryRead = (path, encoding) => {
            try {
                return FS.readFile(path, { encoding });
            } catch (e) {
                return null;
            }
        }

        if (exitCode === 0) {
            const results = tryRead('success.txt', 'utf8');
            const workspaceZip = tryRead('workspace.zip', 'binary');
            postMessage({
                'cmd': 'results',
                'args': {
                    results,
                    workspaceZip,
                },
            });
        }
    };
};

importScripts('glapd-web.js');
