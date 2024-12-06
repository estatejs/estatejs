namespace Estate.Jayne.Errors
{
    public enum PlatformErrorCode
    {
        UnknownWorkerCreationFailure,
        AccountCreationTemporarilyDisabled,
        MaxWorkerLimitReached,
        UserWorkerLimitReached,
        FailedToCreateAccountWithGoogleIdentity,
        FailedToCreateAccountInDatabase,
        FailedToSetAdminKeyInKeyCache,
        FailedToSetUserKeyInKeyCache,
        UnableToGetUserId,
        UnknownWorkerUpdateFailure,
        FailedToDeleteAccountWithGoogleIdentity,
        InternalCommunicationFailure,
        FailedToSetAccountCreationTokenInKeyCache,
        FailedToGetAccountFromGoogleIdentity,
        FailedToSetEmailVerifiedInKeyCache
    }
}