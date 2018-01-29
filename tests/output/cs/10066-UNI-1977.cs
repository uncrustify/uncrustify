// We specifically avoid our own tab-space in GenericFormat because of the @"" issue.
// So we must rely on Uncrustify getting it right, and it nearly does - except for the
// "Layout has changed, bail out now" where it does not replace the tab preceding the comment.

namespace Namespace
{
    class Class
    {
        public void Foo()
        {
            if (bar)
            {
                //  Layout has changed, bail out now.
                bar = false;
            }
        }
    }
}
