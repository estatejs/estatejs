// @flow
import { useEffect, useState, useMemo } from 'react';

import { FirebaseHelper} from "../helpers/firebase-helper";

const useUser = (): { user: any | void, ... } => {
    const [user, setuser] = useState();

    useEffect(() => {
        if (FirebaseHelper.default().isUserLoggedIn()) {
            setuser(FirebaseHelper.default().getLoggedInUser());
        }
    }, []);

    return { user };
};

export default useUser;
