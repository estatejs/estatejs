import React from 'react';
import { Route, Redirect } from 'react-router-dom';
import {FirebaseHelper} from "../helpers/firebase-helper";

/**
 * Private Route forces the authorization before the route can be accessed
 * @param {*} param0
 * @returns
 */
const PrivateRoute = ({ component: Component, roles, ...rest }) => {
    return (
        <Route
            {...rest}
            render={(props) => {
                if (!FirebaseHelper.default().isUserLoggedIn()) {
                    // not logged in so redirect to login page with the return url
                    return <Redirect to={{ pathname: '/account/login', state: { from: props.location } }} />;
                }

                /* TODO: if we ever add roles, restrict page access here
                const loggedInUser = FirebaseAuthHelper.default().getLoggedInUser();
                // check if route is restricted by role
                if (roles && roles.indexOf(loggedInUser.role) === -1) {
                    // role not authorised so redirect to home page
                    return <Redirect to={{ pathname: '/' }} />;
                }*/

                // authorised so return component
                return <Component {...props} />;
            }}
        />
    );
};

export default PrivateRoute;
