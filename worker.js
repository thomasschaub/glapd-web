function renderIndex(genomes) {
    contents = '';
    
    for (const genome of genomes) {
        contents += '>' + genome.name + '\n' + genome.sequence + '\n';
    }

    return contents;
}

function writeIndex(genomes) {
    contents = renderIndex(genomes);
    FS.writeFile('inputs/index.fa', contents);
}

function renderRef(genome) {
    return '>' + genome.name + '\n' + genome.sequence + '\n';
}

function writeRef(genome) {
    const contents = renderRef(genome);
    FS.writeFile('inputs/ref.fa', contents);
}

function renderGenomeNames(genomes) {
    contents = '';
    
    for (const genome of genomes) {
        contents += '>' + genome.name + '\n';
    }

    return contents;
}

function writeGenomeNames(genomes, filename) {
    contents = renderGenomeNames(genomes);
    FS.writeFile(filename, contents);
}

// Emscripten Module hooks

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
        // Parse message

        const { indexFileContents, refFileContents, targetListFileContents, maxNumMismatchesInTarget, backgroundMode, backgroundListFileContents, maxNumMismatchesInBackground, includeLoopPrimers, numPrimersToGenerate } = e.data;

        // Prepare inputs

        FS.mkdir('inputs')
        FS.writeFile('inputs/index.fa', indexFileContents);
        FS.writeFile('inputs/ref.fa', refFileContents);
        if (targetListFileContents)
            FS.writeFile('inputs/target.fa', targetListFileContents);
        if (backgroundListFileContents)
            FS.writeFile('inputs/background.fa', backgroundListFileContents);

        const args = [
            '--index', 'inputs/index.fa',
            '--ref', 'inputs/ref.fa',
            // --target is handled below
            '--maxNumMismatchesInTarget', maxNumMismatchesInTarget.toString(),
            '--backgroundMode', backgroundMode,
            // --backgroundListPath is handled below
            '--maxNumMismatchesInBackground', maxNumMismatchesInBackground.toString(),
            // --includeLoopPrimers is handled below
            '--numPrimersToGenerate', numPrimersToGenerate.toString(),
            '--numThreads', '1', // TODO fix pthreads, then use: String(navigator.hardwareConcurrency),
        ];
        
        if (targetListFileContents)
            args.push('--target', 'inputs/target.fa');

        if (backgroundMode == 'fromFile')
            args.push('--backgroundListPath', 'inputs/background.fa');

        if (includeLoopPrimers)
            args.push('--includeLoopPrimers');

        console.log('About to launch GLAPD', args);

        const exitCode = callMain(args);

        console.log(`GLAPD exit code: ${exitCode}`);

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

importScripts('portable-glapd.js');
