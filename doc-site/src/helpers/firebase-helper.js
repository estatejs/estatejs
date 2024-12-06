import {initializeApp} from 'firebase/app';
import {getAnalytics} from 'firebase/analytics';
// import {getAuth, onAuthStateChanged, signInWithEmailAndPassword, signOut, sendPasswordResetEmail} from "firebase/auth";
import {FIREBASE_CONFIG} from "../config";

function wrapFirebaseError(error) {
    /*let msg = "Unknown authentication error";
    if(error && error.code) {
        switch(error.code) {
            case "auth/invalid-email":
                msg = "Invalid email address";
                break;
            case "auth/wrong-password":
            case "auth/user-not-found":
                msg = "Invalid username or password";
                break;
            default:
                console.error(`Unknown firebase error encountered: ${JSON.stringify(error)}`)
                break;
        }
    }
    return new Error(msg);*/
}

export const FirebaseHelper = (function () {
    class InternalFirebaseHelper {
        constructor() {
            console.log("Initializing firebase app");
            this._app = initializeApp(FIREBASE_CONFIG);
            if(!this._app) {
                throw new Error("unable to init firebase app");
            }
            console.log("Initializing firebase analytics");
            this._analytics = getAnalytics(this._app);
            if(!this._analytics){
                throw new Error('unable to init analytics');
            }

            /*const auth = getAuth(this._app);
            if(!auth)
                throw new Error("unable to init firebase auth");
            onAuthStateChanged(auth, (user) => {
                if(user) {
                    console.log("Auth state changed, user exists now");
                } else {
                    console.log("Auth state changed, user no longer exists");
                }
                this.setLoggedInUser(user);
            });*/
        }

        isUserLoggedIn() {
            /*return !!this.getLoggedInUser();*/
        }

        // See https://firebase.google.com/docs/reference/js/firebase.User for properties on User
        getLoggedInUser() {
            /*return sessionStorage.getItem("firebase_user");*/
        }

        setLoggedInUser(user) {
            /*if (!user) {
                sessionStorage.removeItem("firebase_user");
            } else {
                sessionStorage.setItem("firebase_user", user);
            }*/
        }

        async loginAndGetUserAsync(email: string, password: string) {
            /*console.log("Logging in");
            const auth = getAuth(this._app);
            try
            {
                const userCredential = await signInWithEmailAndPassword(auth, email, password);
                if (userCredential && userCredential.user) {
                    console.log("Logged in successfully");
                    return userCredential.user;
                }
            }
            catch(error) {
                throw wrapFirebaseError(error);
            }*/
        }

        async logoutAsync() {
            /*console.log("Signing out");
            const auth = getAuth(this._app);
            await signOut(auth);
            console.log("Signed out");*/
        }

        async sendResetPasswordEmailAsync(email: string) {
            /*console.log("Sending password reset email");
            const auth = getAuth(this._app)
            await sendPasswordResetEmail(auth, email);
            console.log("Successfully sent password reset email");*/
        }
    }

    let instance;
    return {
        default: function () {
            if (instance == null) {
                instance = new InternalFirebaseHelper();
                // Hide the constructor so the returned object can't be new'd...
                instance.constructor = null;
            }
            return instance;
        },
        init: function () {
          this.default()
        }
    };
})();