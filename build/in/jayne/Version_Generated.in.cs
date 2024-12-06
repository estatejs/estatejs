namespace Estate.Jayne
{
    public class Version
    {
        public const uint Major = {{ESTATE_MAJOR_VERSION}};
        public const uint Minor = {{ESTATE_MINOR_VERSION}};
        public const uint Patch = {{ESTATE_PATCH_VERSION}};
        public const string Build = "{{ESTATE_BUILD_VERSION}}";
        public const uint SiteProtocolVersion = {{ESTATE_JAYNE_SITE_PROTOCOL_VERSION}};
        public const uint ToolsProtocolVersion = {{ESTATE_JAYNE_TOOLS_PROTOCOL_VERSION}};
        public const string String = "{{ESTATE_MAJOR_VERSION}}.{{ESTATE_MINOR_VERSION}}.{{ESTATE_PATCH_VERSION}}-{{ESTATE_BUILD_VERSION}}";
    }
}