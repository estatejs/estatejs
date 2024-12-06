// @flow
import React from 'react';
import Routes from './routes/Routes';
import {FirebaseHelper} from "./helpers/firebase-helper";

// Themes
import './assets/scss/Saas-Dark.scss';
import './assets/css/prism.css';

type AppProps = {};

// Initialize Firebase services
FirebaseHelper.init()

/**
 * Main app component
 */
const App = (props: AppProps): React$Element<any> => {
    return <Routes></Routes>;
};

export default App;
