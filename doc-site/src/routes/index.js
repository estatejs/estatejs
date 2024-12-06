import React from 'react';
import {Redirect} from 'react-router-dom';
import {Route} from 'react-router-dom';
// NOTE: No private routes because auth isn't enabled
// import PrivateRoute from './PrivateRoute';

const Console = React.lazy(() => import('../pages/console'));
const FinalizeAccount = React.lazy(() => import('../pages/finalize-account'));

const Overview = React.lazy(() => import('../pages/documentation/overview'));
const Handbook = React.lazy(() => import('../pages/documentation/handbook'));

const Contact = React.lazy(() => import('../pages/contact'));

const ExerciseTracker = React.lazy(() => import('../pages/examples/exercise-tracker'));

const Community = React.lazy(() => import('../pages/community'));

const ErrorPageNotFound = React.lazy(() => import('../pages/system/page-not-found'));
const ServerError = React.lazy(() => import('../pages/system/server-error'));
const Maintenance = React.lazy(() => import('../pages/system/maintenance'));

const Login = React.lazy(() => import('../pages/account/login'));
const Logout = React.lazy(() => import('../pages/account/logout'));
const ForgotPassword = React.lazy(() => import('../pages/account/reset-password'));
const MyAccount = React.lazy(() => import('../pages/account/my-account'));

// By default redirect to the quick start guide
const DefaultRoute = (): React$Element<React$FragmentType> => {
    return (<Redirect to={'/documentation/overview'} />);
};

const rootRoute = {
    path: '/',
    exact: true,
    component: DefaultRoute,
    route: Route,
};

const consoleRoute = {
    path: '/console',
    exact: true,
    component: Console,
    route: Route
}

const finalizeAccountRoute = {
    path: '/finalize-account',
    exact: true,
    component: FinalizeAccount,
    route: Route
}

const contactRoute = {
    path: '/contact',
    exact: true,
    component: Contact,
    route: Route
}

const documentationRoute = {
    path: '/documentation',
    name: 'Documentation',
    header: 'Developer Resources',
    icon: 'uil-book-alt',
    children: [
        {
            path: '/documentation/overview',
            name: 'Overview',
            component: Overview,
            route: Route
        },
        {
            path: '/documentation/handbook',
            name: 'Handbook',
            component: Handbook,
            route: Route
        },
        {
            path: '/documentation/quick-start',
            name: 'Quick Start',
            component: ExerciseTracker,
            route: Route
        }
    ]
};

const examplesRoute = {
    path: '/examples',
    name: 'Examples',
    icon: 'uil-lightbulb-alt',
    children: [
        {
            path: '/examples/exercise-tracker',
            name: 'Exercise Tracker (React)',
            component: ExerciseTracker,
            route: Route
        }
    ]
};

const communityRoute = {
    path: '/community',
    exact: true,
    component: Community,
    route: Route
};

const systemRoutes = [
    {
        path: '/maintenance',
        name: 'Maintenance',
        component: Maintenance,
        route: Route,
    },
    {
        path: '/error-404',
        name: 'Error - 404',
        component: ErrorPageNotFound,
        route: Route,
    },
    {
        path: '/error-500',
        name: 'Error - 500',
        component: ServerError,
        route: Route,
    }
];

const accountRoutes = [
    {
        path: '/account/login',
        name: 'Login',
        component: Login,
        route: Route,
    },
    {
        path: '/account/logout',
        name: 'Logout',
        component: Logout,
        route: Route,
    },
    {
        path: '/account/forgot-password',
        name: 'Forgot Password',
        component: ForgotPassword,
        route: Route,
    },
    {
        path: '/account/my-account',
        name: 'My Account',
        component: MyAccount,
        route: Route,
    }
];

// flatten the list of all nested routes
const flattenRoutes = (routes) => {
    let flatRoutes = [];

    routes = routes || [];
    routes.forEach((item) => {
        flatRoutes.push(item);

        if (typeof item.children !== 'undefined') {
            flatRoutes = [...flatRoutes, ...flattenRoutes(item.children)];
        }
    });
    return flatRoutes;
};


// All routes
const authProtectedRoutes = [];
const publicRoutes = [rootRoute, consoleRoute, finalizeAccountRoute, contactRoute, documentationRoute, examplesRoute, communityRoute, ...systemRoutes, ...accountRoutes];

const authProtectedFlattenRoutes = flattenRoutes([...authProtectedRoutes]);
const publicProtectedFlattenRoutes = flattenRoutes([...publicRoutes]);

export {publicRoutes, authProtectedRoutes, authProtectedFlattenRoutes, publicProtectedFlattenRoutes};
