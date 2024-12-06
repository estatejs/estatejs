let LOG_PREFIX = null;

function setLogPrefix(prefix) {
    LOG_PREFIX = prefix;
}

function getLogPrefix() {
    return LOG_PREFIX ? LOG_PREFIX : "";
}

async function test(test_name, func) {
    console.log(`${getLogPrefix()}++ Test: ` + test_name);
    await func();
    console.log(`${getLogPrefix()}== OK`);
}

function run(suite, shutdown) {
    suite().then(value => {
        shutdown();
    }).then(value => {
        // don't change this. test-browser.sh looks for it to know everything worked.
        // Also, the reason I'm not just using process.exit(1) is because browserify freezes if I use process.
        console.log(`${getLogPrefix()}####### All tests completed successfully #######`);
        if (typeof window !== 'undefined') {
            window.close();
        }
    }).catch(reason => {
        shutdown();
        console.error(`${getLogPrefix()}!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!`);
        console.error(`${getLogPrefix()}!!!!!!! Failure: ${reason} !!!!!!!`);
        console.trace(reason);
        if (typeof window !== 'undefined') {
            window.close();
        }
        process.exitCode = 1;
    });
}

module.exports = {test, run, getLogPrefix, setLogPrefix};