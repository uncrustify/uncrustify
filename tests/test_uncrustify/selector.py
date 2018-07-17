# Utility class to match test filters.
#
# * @author  Matthew Woehlke    June 2018
#


# =============================================================================
class Selector(object):
    # -------------------------------------------------------------------------
    def __init__(self, s):
        class Range(object):
            pass

        self.ranges = []
        for p in s.split(','):
            r = Range()
            if ':' in p:
                r.group, p = p.split(':')
            else:
                r.group = None
            if '-' in p:
                r.lower, r.upper = map(int, p.split('-'))
            else:
                r.lower = int(p)
                r.upper = int(p)
            self.ranges.append(r)

    # -------------------------------------------------------------------------
    def test(self, name):
        group, num = name.split(':')
        num = int(num)

        for r in self.ranges:
            if r.group is not None and r.group != group:
                continue
            if num < r.lower or num > r.upper:
                continue
            return True

        return False
