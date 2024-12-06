import { system, createUuid, Data, Message, Service } from "./worker-runtime";

export class Exercise extends Data {
    constructor(public user: User, public description: string, public duration: number, public date: Date) {
        super(createUuid(false));
    }
}

export class ExerciseAdded extends Message {
    constructor(public exercise: Exercise) {
        super();
    }
}

export class User extends Data {
    constructor(public username: string) {
        super(username);
    }
}

export class ExerciseTrackerService extends Service {
    private _userIndex: Set<string>;
    private _exerciseIndex: Set<string>;

    constructor(primaryKey: string) {
        super(primaryKey);
        this._userIndex = new Set();
        this._exerciseIndex = new Set();
    }

    deleteUser(user: User) {
        if (!user || !(user instanceof User)) {
            throw new Error('User was invalid');
        }
        this._userIndex.delete(user.primaryKey);
        system.delete(user);
    }

    addUser(username: string): User {
        if (this._userIndex.has(username)) {
            throw new Error("A user with that name already exists");
        }
        const user = new User(username);
        system.saveData(user);
        this._userIndex.add(user.primaryKey);
        return user;
    }

    userExists(username: string): boolean {
        return this._userIndex.has(username);
    }

    tryGetUser(username: string): User | null {
        if (this._userIndex.has(username))
            return system.getData(User, username);
        return null;
    }

    getUsers(): User[] {
        let users: User[] = [];
        for (const pk of this._userIndex)
            users.push(system.getData(User, pk));
        return users;
    }

    addExercise(exercise: Exercise) {
        if (!exercise || !(exercise instanceof Exercise)) {
            throw new Error('Exercise was invalid');
        }
        if (this._exerciseIndex.has(exercise.primaryKey)) {
            throw new Error(`Exercise ${exercise.primaryKey} already exists`);
        }
        system.saveData(exercise);
        this._exerciseIndex.add(exercise.primaryKey);
        system.sendMessage(this, new ExerciseAdded(exercise));
    }

    deleteExercise(primaryKey: string) {
        const exercise = system.getData(Exercise, primaryKey);
        if (exercise) {
            this._exerciseIndex.delete(exercise.primaryKey);
            system.delete(exercise);
        } else {
            throw new Error('Failed to delete exercise because it does not exist');
        }
    }

    getMaxDuration(): number | null {
        let max: number | null = null;
        for (const pk of this._exerciseIndex) {
            const exercise = system.getData(Exercise, pk);
            if (!max || exercise.duration > max)
                max = exercise.duration;
        }
        return max;
    }

    getExercises(): Exercise[] {
        let exercises: Exercise[] = [];
        for (const pk of this._exerciseIndex) {
            exercises.push(system.getData(Exercise, pk));
        }
        return exercises;
    }
}
