let SERVICE;

async function suite() {
    setLogPrefix("[suite]");
    worker.setOptions(new WorkerOptions(true, true, getLogPrefix()));

    await test('Can create Service', async function () {
        SERVICE = worker.getService(MemoryCapService, "default");
        if (!SERVICE)
            throw new Error("couldn't get the service");
    });

    await test('Can explode', async function () {
        let thrown = false;
        try{
            await SERVICE.explodeAsync();
        }
        catch(e) {
            if(e instanceof PlatformError) {
                if (e.code === "Innerspace_ConnectionFault") {
                    thrown = true;
                    console.log(`Received the expected result`);
                } else {
                    console.log(`Recieved the right exception but an unexpected code ${e.code} with message ${e.message}`);
                }
            } else {
                console.log(`Failed to receive the right exception object. Raw exception: ${JSON.stringify(e)}`);
            }
        }
        if(!thrown)
            throw new Error("Failed to throw from method call that should have crashed the service.");
    });
}

run(suite, () => {
    worker.closeConnections();
});