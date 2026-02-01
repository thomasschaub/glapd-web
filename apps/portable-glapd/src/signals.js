addToLibrary({
    notify_about_to_start_phase: function(phase) {
        postMessage({
            'cmd': 'notify_about_to_start_phase',
            'phase': UTF8ToString(phase),
        });
    }
});