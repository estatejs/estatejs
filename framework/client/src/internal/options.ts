import {WorkerOptions} from "../public/worker-options.js";

class Options {
    private _workerOptions: WorkerOptions | null = null;

    setWorkerOptions(workerOptions: WorkerOptions) {
        this._workerOptions = workerOptions;
    }

    getWorkerOptions(): WorkerOptions {
        if (!this._workerOptions)
            this._workerOptions = new WorkerOptions();
        return this._workerOptions;
    }
}

export const options = new Options();
