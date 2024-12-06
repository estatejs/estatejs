// @flow
import React, {useEffect} from 'react';
import {Redirect} from "react-router-dom";
import {useQuery} from "../hooks";
import {JAYNE_URL, SITE_PROTOCOL_VERSION} from "../config";

const crypto = require("crypto");

const LOG_CONTEXT_LENGTH = 10;
function createLogContext() {
    return crypto.randomBytes(LOG_CONTEXT_LENGTH / 2).toString('hex');
}

async function finalizeAccountOnceAsync(token) {
    const data = {
        logContext: createLogContext(),
        accountCreationToken: token
    };
    return await fetch(`${JAYNE_URL}/site-account/finalize_account_once`, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'X-Estate-Site-Protocol-Version': SITE_PROTOCOL_VERSION
        },
        body: JSON.stringify(data),
    });
}

function FinalizeAccountPage() {
    const query = useQuery();
    const token = query.get('token');
    useEffect(() => {
            if (!token)
                return;
            const finalizeAccount = async () => {
                if (token) {
                    console.log("Finalizing account with Jayne");
                    await finalizeAccountOnceAsync(token);
                    console.log("Estate account created successfully")
                } else {
                    console.error("Invalid token");
                }
            };
            finalizeAccount().then();
        },
        [token]);
    return (
        <Redirect to='/documentation/overview'/>
    );
}

export default FinalizeAccountPage;
