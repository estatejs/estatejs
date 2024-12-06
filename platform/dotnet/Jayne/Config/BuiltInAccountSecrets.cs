namespace Estate.Jayne.Config
{
    public class BuiltInAccount
    {
        public string UserId { get; set; }
        public string Email { get; set; }
        public string Password { get; set; }
    }
    
    public class BuiltInAccountSecrets
    {
        public BuiltInAccount[] Accounts { get; set; }
    }
}