using Estate.Jayne.Common;

namespace Estate.Jayne.Models
{
    public readonly struct UserProfile
    {
        public UserProfile(Username username)
        {
            Requires.NotDefault(nameof(username), username);
            Username = username;
        }

        public Username Username { get; }

        public override string ToString()
        {
            return $"{nameof(Username)}: {Username}";
        }
    }
}