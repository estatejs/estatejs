let SERVICE;
let USER;
let EXERCISE;

const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));

const MESSAGE_RECEIVE_SLEEP_MS = 1000;
const UPDATE_RECEIVED_SLEEP_MS = 1000;

async function suite0() {
    setLogPrefix("[suite0======]");
    worker.setOptions(new WorkerOptions(true, true, getLogPrefix()));

    await test('Can round-trip user without unsaved changes error', async function () {
        const service = worker.getService(ExerciseTrackerService, createUuid(false));
        if (!service)
            throw new Error("Failed to get service");
        const user = await service.addUserAsync(createUuid(false));
        if (!user)
            throw new Error("Failed to add user");
        const exercise = new Exercise(user, "Just testing", 100, new Date());

        let messageReceived = false;
        await worker.subscribeMessageAsync(service, ExerciseAdded, async (message) => {
            if (message.exercise.primaryKey !== exercise.primaryKey)
                throw new Error("Unexpected exercise added");
            messageReceived = true;
            await worker.unsubscribeMessageAsync(service, ExerciseAdded);
        });

        await service.addExerciseAsync(exercise);

        if (!messageReceived)
            throw new Error("Never received exercise added message");
    });

    await test('Can create ExerciseTrackerService', async function () {
        SERVICE = worker.getService(ExerciseTrackerService, "default");
        if (!SERVICE)
            throw new Error("couldn't get the service");
    });

    await test('Can add User', async function () {
        const username = createUuid(false);
        USER = await SERVICE.addUserAsync(username);
        if (!USER)
            throw new Error("Returned user was empty");
        if (USER.username !== username)
            throw new Error("Username was: " + USER.username);
    });

    await test('Can see if user exists by username', async function () {
        const exists = await SERVICE.userExistsAsync(USER.username);
        if (!exists)
            throw new Error('User did not exist');
    });

    await test('Can get user by username', async function () {
        const comp = await SERVICE.tryGetUserAsync(USER.username);
        if (!(comp instanceof User))
            throw new Error('comp was not a user instance');
        if (comp.username !== USER.username)
            throw new Error('comp did not have the same username');
    });

    await test('Can get all users', async function () {
        const users = await SERVICE.getUsersAsync();
        let found = false;
        for (let user of users) {
            if (!(user instanceof User))
                throw new Error("user was not an instance of User");
            if (user.username === USER.username)
                found = true;
        }
        if (!found)
            throw new Error("User added wasn't present in the returned users");
    });

    await test('Can add exercise', async function () {
        let messageReceived = false;
        let exercise = new Exercise(USER, 'I worked out', 100, new Date());

        await worker.subscribeMessageAsync(SERVICE, ExerciseAdded, async (message) => {
            if (message.exercise.primaryKey !== exercise.primaryKey)
                throw new Error("Unexpected exercise added");
            messageReceived = true;
            EXERCISE = exercise;
            await worker.unsubscribeMessageAsync(SERVICE, ExerciseAdded);
        });

        //Wait for suite1 to subscribe to the message before calling the service method which fires the message
        await barrierWaitAsync('SUITE1-HAS-SUBSCRIBED-EXERCISE-ADDED');

        await SERVICE.addExerciseAsync(exercise);

        //Tell suite1 the message has been fired and it should have recieved the message now.
        await barrierWaitAsync('SUITE0-HAS-FIRED-EXERCISE-ADDED');

        if (!messageReceived)
            throw new Error("Suite0 didn't receive the message.");
    });

    await test('Can delete exercise', async function () {
        let exercises = await SERVICE.getExercisesAsync();

        // wait for suite1 to subscribe before deleting the objects.
        await barrierWaitAsync('SUITE1-HAS-SUBSCRIBED-EXERCISE-UPDATE');

        for (let exercise of exercises) {
            await SERVICE.deleteExerciseAsync(exercise.primaryKey);
        }

        // tell suite1 the exercises have been deleted and it should have received the updates by now.
        await barrierWaitAsync('SUITE0-HAS-DELETED-EXERCISES');

        exercises = await SERVICE.getExercisesAsync();
        if (exercises.length !== 0)
            throw new Error("Invalid number of exercises after delete");
    });

    await test('Can delete created user', async function () {
        await SERVICE.deleteUserAsync(USER);
        const exists = await SERVICE.userExistsAsync(USER.username);
        if (exists)
            throw new Error('User existed after delete');
    });
}

async function suite1() {
    setLogPrefix("[======suite1]");
    worker.setOptions(new WorkerOptions(true, true, getLogPrefix()));

    await test('Can get service', async function () {
        SERVICE = worker.getService(ExerciseTrackerService, "default");
        if (!SERVICE)
            throw new Error("couldn't get the service");
    });

    await test('Can receive ExerciseAdded message', async function () {
        let messageReceived = false;

        await worker.subscribeMessageAsync(SERVICE, ExerciseAdded, async (message) => {
            if (!message instanceof ExerciseAdded)
                throw new Error("message was not an ExerciseAdded")
            if (!message.exercise instanceof Exercise)
                throw new Error("message.exercise was not an Exercise");
            messageReceived = true;
            await worker.unsubscribeMessageAsync(SERVICE, ExerciseAdded);
        });

        // Tell suite0 it's OK to fire the message now.
        await barrierWaitAsync('SUITE1-HAS-SUBSCRIBED-EXERCISE-ADDED');

        // Wait for suite0 to have fired the exercise added message
        await barrierWaitAsync('SUITE0-HAS-FIRED-EXERCISE-ADDED');

        if(!messageReceived) {
            await sleep(MESSAGE_RECEIVE_SLEEP_MS);
        }

        if (!messageReceived)
            throw new Error("Suite1 didn't receive the message.");
    })

    await test('Can receive deleted Exercise object update message', async function () {
        const exercises = await SERVICE.getExercisesAsync();
        await worker.subscribeUpdatesAsync(exercises);
        let toDelete = new Set(exercises);
        let handled = false;
        for (let exercise of exercises) {
            worker.addUpdateListener(exercise, e => {
                handled = true;
                if (e.deleted) {
                    if (!toDelete.delete(e.target))
                        throw new Error("Deleted object that wasn't supposed to exist");
                } else {
                    throw new Error("Unexpected non-delete object update received");
                }
            });
        }

        // Tell suite0 it's OK to delete the exercises now because we're ready to receive the updates.
        await barrierWaitAsync('SUITE1-HAS-SUBSCRIBED-EXERCISE-UPDATE');

        // wait for suite0 to have deleted the exercises
        await barrierWaitAsync('SUITE0-HAS-DELETED-EXERCISES');

        if(!handled) {
            await sleep(UPDATE_RECEIVED_SLEEP_MS);
        }

        if (!handled)
            throw new Error("No object update message was handled by the listener");

        if (toDelete.size > 0)
            throw new Error(`Not all object updates were recieved. ${toDelete.size} missing`);
    });
}

run([suite0, suite1], () => {
    worker.closeConnections();
});
