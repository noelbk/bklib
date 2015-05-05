def fixlen(n, l, append = [None]):
    """
    Return a list of exactly length n by truncating or appending to l.
    
    List assignment is great for functions like split(), but it's very
    strict that len(lhs) == len(rhs), which isn't always true depeding
    on the string input to split.  This is safe:

    (a,b,c) = fixlen(3, split(input))
    """

    if len(l) > n:
        return l[0:n];
    elif len(l) < n:
        return l + append * (n - len(l))
    return l
