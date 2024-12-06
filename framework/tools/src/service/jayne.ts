import {GoogleIdentity, GoogleIdentitySingleton} from "./google-identity";
import {requiresTruthy} from "../util/requires";
import {ESTATE_SERVER_ERROR_CODE, parseError, ESTATE_EMAIL_NOT_VERIFIED_ERROR_CODE} from "../model/jayne-error";
import fetch from 'cross-fetch';
import {WorkerClassMapping} from "../model/worker-config";
import {JAYNE_SERVICE_URL, JAYNE_TOOLS_PROTOCOL_VERSION} from "../config";

//-- REQUEST

class BeginCreateAccountRequest {
    constructor(public logContext: string, public username: string, public password: string) {
        requiresTruthy('logContext', logContext);
        requiresTruthy('username', username);
        requiresTruthy('password', password);
    }
}

class DeployWorkerRequest {
    constructor(public logContext: string,
                public workerName: string,
                public language: number,
                public workerFiles: WorkerFileContent[],
                public compressedWorkerTypeDefinitionsStr: string,
                public workerIdStr?: string,
                public currentWorkerVersionStr?: string,
                public workerClassMappings?: WorkerClassMapping[],
                public lastClassId?: number) {
        requiresTruthy('logContext', logContext);
        requiresTruthy('workerName', workerName);
        if (language !== 0)
            throw new Error("invalid worker language");
        requiresTruthy('workerFiles', workerFiles);
        requiresTruthy('compressedWorkerTypeDefinitionsStr', compressedWorkerTypeDefinitionsStr);
    }
}

class DeleteWorkerRequest {
    constructor(public logContext: string,
                public workerName: string) {
        requiresTruthy('logContext', logContext);
        requiresTruthy('workerName', workerName);
    }
}

export class WorkerFileContent {
    constructor(public code: string, public name: string) {
        requiresTruthy('code', code);
        requiresTruthy('name', name);
    }
}

//-- RESPONSE

class ListWorkersResponse {
    constructor(public ownedWorkers: string[]) {
        requiresTruthy('ownedWorkers', ownedWorkers);
    }
}

class IsEmailVerifiedResponse {
    constructor(public emailVerified: boolean) {
    }
}

class BeginCreateAccountResponse {
    constructor(public emailVerified: boolean,
                public accountCreationToken: string,
                public expiresInSeconds: number) {
        if(this.emailVerified) {
            requiresTruthy('!this.accountCreationToken', !this.accountCreationToken);
            requiresTruthy('!this.expiresInSeconds', !this.expiresInSeconds);
        } else {
            requiresTruthy('this.accountCreationToken', this.accountCreationToken);
            requiresTruthy('this.expiresInSeconds', this.expiresInSeconds);
        }
    }
}

class LoginExistingAccountResponse {
    constructor(public adminKey: string) {
        requiresTruthy('adminKey', adminKey);
    }
}

class DeployWorkerResponse {
    constructor(public workerIdStr: string, public workerVersionStr: string, public workerClassMappings: WorkerClassMapping[]) {
        requiresTruthy('workerIdStr', workerIdStr);
        requiresTruthy('workerVersionStr', workerVersionStr);
        requiresTruthy('workerClassMappings', workerClassMappings)
    }
}

class GetWorkerUseInfoResponse {
    constructor(public workerIndexStr: string, public userKey: string, public compressedWorkerTypeDefinitionsStr: string) {
        requiresTruthy('workerIndexStr', workerIndexStr);
        requiresTruthy('userKey', userKey);
        requiresTruthy('compressedWorkerTypeDefinitionsStr', compressedWorkerTypeDefinitionsStr);
    }
}

const ESTATE_JAVASCRIPT_LANGUAGE = 0;

export class Jayne {
    constructor(private googleIdentity: GoogleIdentity) {
        requiresTruthy('googleIdentity', googleIdentity);
    }

    public async deleteAccountAsync(logContext: string): Promise<void> {
        try {
            let fetchInit = this.createGoogleIdentityFetchInit("DELETE", "delete your Estate account and all its data");
            await this.makeServiceCallAsync(fetchInit, `tools-account/delete_account?logContext=${logContext}`, false, true);
        } catch (e: any) {
            if (e.code === "ECONNREFUSED")
                throw new Error("Failed because the connection was refused");
            throw new Error(e.message);
        }
    }

    public async listWorkersAsync(logContext: string, adminKey: string): Promise<ListWorkersResponse> {
        try {
            let fetchInit = this.createAdminKeyFetchInit("GET", adminKey);
            let json = await this.makeServiceCallAsync(fetchInit, `tools-worker-admin/list_workers?logContext=${logContext}`, true, true);
            return new ListWorkersResponse(json.ownedWorkers);
        } catch (e: any) {
            if (e.code === "ECONNREFUSED")
                throw new Error("Failed because the connection was refused");
            throw new Error(e.message);
        }
    }

    public async deleteWorkerAsync(logContext: string, adminKey: string, workerName: string): Promise<void> {
        const req = new DeleteWorkerRequest(logContext, workerName);

        try {
            let fetchInit = this.createAdminKeyFetchInit("POST", adminKey);
            await this.makeServiceCallAsync(fetchInit, "tools-worker-admin/delete_worker", false, true, req);
        } catch (e: any) {
            if (e.code === "ECONNREFUSED")
                throw new Error("Failed to because the connection was refused");
            throw new Error(e.message);
        }
    }

    public async deployWorkerAsync(logContext: string,
                               adminKey: string,
                               workerName: string,
                               workerFiles: WorkerFileContent[],
                               compressedWorkerTypeDefinitionsStr: string,
                               workerClassMappings?: WorkerClassMapping[],
                               workerId?: string,
                               currentWorkerVersion?: string,
                               lastClassId?: number): Promise<DeployWorkerResponse> {
        const req = new DeployWorkerRequest(logContext, workerName, ESTATE_JAVASCRIPT_LANGUAGE,
            workerFiles, compressedWorkerTypeDefinitionsStr, workerId, currentWorkerVersion, workerClassMappings, lastClassId);

        try {
            let fetchInit = this.createAdminKeyFetchInit("POST", adminKey);
            let jsonObj = await this.makeServiceCallAsync(fetchInit, "tools-worker-admin/deploy_worker", true, true, req);
            return new DeployWorkerResponse(jsonObj.workerIdStr, jsonObj.workerVersionStr, jsonObj.workerClassMappings);
        } catch (e: any) {
            if (e.code === "ECONNREFUSED")
                throw new Error("Failed because the connection was refused");
            throw new Error(e.message);
        }
    }

    public async isEmailVerifiedAsync(logContext: string, accountCreationToken: string): Promise<boolean> {
        try {
            let fetchInit = this.createUnauthenticatedFetchInit("GET");
            const response = await this.makeServiceCallAsync(fetchInit, `tools-account/is_email_verified?accountCreationToken=${accountCreationToken}`, false, false);
            return response.status === 200;
        } catch (e: any) {
            if (e.code === "ECONNREFUSED")
                throw new Error("Failed because the connection was refused");
            throw new Error(e.message);
        }
    }

    public async beginCreateAccountAsync(logContext: string, username: string, password: string): Promise<BeginCreateAccountResponse> {
        const req = new BeginCreateAccountRequest(logContext, username, password);

        try {
            let fetchInit = this.createUnauthenticatedFetchInit("POST");
            let jsonObj = await this.makeServiceCallAsync(fetchInit, "tools-account/begin_create_account", true, true, req);
            return new BeginCreateAccountResponse(jsonObj.emailVerified, jsonObj.accountCreationToken, jsonObj.expiresInSeconds);
        } catch (e: any) {
            if (e.code === "ECONNREFUSED")
                throw new Error("Failed because the connection was refused");
            throw new Error(e.message);
        }
    }

    public async loginExistingAccountAsync(logContext: string): Promise<LoginExistingAccountResponse> {
        try {
            let fetchInit = this.createGoogleIdentityFetchInit("GET", "login existing account");
            let jsonObj = await this.makeServiceCallAsync(fetchInit, `tools-account/login_existing_account?logContext=${logContext}`, true, true);
            return new LoginExistingAccountResponse(jsonObj.adminKey);
        } catch (e: any) {
            if (e.code === "ECONNREFUSED")
                throw new Error("Failed because the connection was refused");
            throw new Error(e.message);
        }
    }

    public async getWorkerConnectionInfoAsync(logContext: string, adminKey: string, workerName: string): Promise<GetWorkerUseInfoResponse> {
        try {
            let fetchInit = this.createAdminKeyFetchInit("GET", adminKey);
            let jsonObj = await this.makeServiceCallAsync(fetchInit, `tools-worker-admin/get_worker_connection_info/${workerName}?logContext=${logContext}`, true, true);
            return new GetWorkerUseInfoResponse(jsonObj.workerIndexStr, jsonObj.userKey, jsonObj.compressedWorkerTypeDefinitionsStr);
        } catch (e: any) {
            if (e.code === "ECONNREFUSED")
                throw new Error("Failed because the connection was refused");
            throw new Error(e.message);
        }
    }

    private assertLoggedIn(what: string) {
        if (!this.googleIdentity.isLoggedIn) {
            throw new Error("Must be logged into Google Identity to " + what);
        }
    }

    private createFetchInit(additional: any) {
        const commonHeaders = {
            "X-Estate-Tools-Protocol-Version": JAYNE_TOOLS_PROTOCOL_VERSION,
            'Content-Type': 'application/json'
        };
        if (additional.headers)
            additional.headers = Object.assign(additional.headers, commonHeaders);
        else
            additional.headers = commonHeaders;
        return additional;
    }

    private createAdminKeyFetchInit(method: string, adminKey: string) {
        return this.createFetchInit({
            method: method,
            headers: {
                'X-WorkerAdminKey': adminKey
            }
        });
    }

    private createGoogleIdentityFetchInit(method: string, what: string) {
        this.assertLoggedIn(what);
        return this.createFetchInit({
            method: method,
            credentials: 'include',
            headers: {
                'Authorization': 'Bearer ' + this.googleIdentity.idToken,
            }
        });
    }

    private createUnauthenticatedFetchInit(method: string) {
        return this.createFetchInit({
            method: method
        });
    }

    private async makeServiceCallAsync(fetchInit: any, path: string, expectResult: boolean, parse: boolean, body?: any): Promise<any> {
        const url = `${JAYNE_SERVICE_URL}/${path}`;

        if (body) { // @ts-ignore
            fetchInit.body = JSON.stringify(body);
        }

        // @ts-ignore
        let response = await fetch(url, fetchInit);

        if(parse) {
            if (!response.ok) {
                if (response.status === ESTATE_SERVER_ERROR_CODE) {
                    throw parseError(await response.json());
                }
                throw new Error("Response was not OK: " + response.statusText);
            }
            if (expectResult) {
                let json = await response.json();
                if (!json)
                    throw new Error("Response didn't return anything");

                return json;
            }
        }

        return response;
    }
}

export const JayneSingleton = new Jayne(GoogleIdentitySingleton);