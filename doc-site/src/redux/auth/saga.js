// @flow
import { all, fork, put, takeEvery, call } from 'redux-saga/effects';
import {FirebaseHelper} from "../../helpers/firebase-helper";
import { authApiResponseSuccess, authApiResponseError } from './actions';
import { AuthActionTypes } from './constants';

/**
 * Login the user
 * @param {*} payload - email and password
 */
function* login({ payload: { email, password } }) {
    try {
        const user = yield FirebaseHelper.default().loginAndGetUserAsync(email, password);
        FirebaseHelper.default().setLoggedInUser(user);
        yield put(authApiResponseSuccess(AuthActionTypes.LOGIN_USER, user));
    } catch (error) {
        yield put(authApiResponseError(AuthActionTypes.LOGIN_USER, error.message));
        FirebaseHelper.default().setLoggedInUser(null);
    }
}

/**
 * Logout the user
 */
function* logout() {
    try {
        yield FirebaseHelper.default().logoutAsync();
        FirebaseHelper.default().setLoggedInUser(null);
        yield put(authApiResponseSuccess(AuthActionTypes.LOGOUT_USER, {}));
    } catch (error) {
        yield put(authApiResponseError(AuthActionTypes.LOGOUT_USER, error));
    }
}

function* forgotPassword({ payload: { email } }) {
    try {
        yield FirebaseHelper.default().sendResetPasswordEmailAsync(email);
        yield put(authApiResponseSuccess(AuthActionTypes.FORGOT_PASSWORD, {}));
    } catch (error) {
        yield put(authApiResponseError(AuthActionTypes.FORGOT_PASSWORD, error));
    }
}

export function* watchLoginUser(): any {
    yield takeEvery(AuthActionTypes.LOGIN_USER, login);
}

export function* watchLogout(): any {
    yield takeEvery(AuthActionTypes.LOGOUT_USER, logout);
}

export function* watchForgotPassword(): any {
    yield takeEvery(AuthActionTypes.FORGOT_PASSWORD, forgotPassword);
}

function* authSaga(): any {
    yield all([
        fork(watchLoginUser),
        fork(watchLogout),
        fork(watchForgotPassword)
    ]);
}

export default authSaga;
