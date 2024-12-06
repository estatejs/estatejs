namespace Estate.Jayne.Errors
{
    public enum BusinessLogicErrorCode
    {
        UnknownParserError,
        InvalidUsername,
        DuplicateWorkerName,
        UserNotFound,
        UserAlreadyExists,
        EmailUnverified,
        UserDisabled,
        MissingCurrentWorkerVersion,
        InvalidWorkerLanguage,
        WorkerNotFoundByName,
        WorkerNotFoundById,
        WorkerNotFoundPullLatest,
        MissingWorkerId,
        InvalidAccountCreationToken,
        MissingTypeDefinitions
    }
}