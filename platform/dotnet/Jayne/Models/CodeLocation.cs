using System;

namespace Estate.Jayne.Models
{
    public readonly struct CodeLocation
    {
        public CodeLocation(int start, int end)
        {
            if (end < start)
                throw new ArgumentException("Invalid value for end: it was less than start");
            if(start < 0)
                throw new ArgumentException("Invalid value for start: it was less than 0");
            if(end < 0)
                throw new ArgumentException("Invalid value for end: it was less than 0");
            
            Start = start;
            End = end;
        }

        public int Start { get; }
        public int End { get; }
    }
}