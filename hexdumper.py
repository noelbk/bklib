# from http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/142812

isbinary_map = [ len(repr(chr(c))) > 4 for c in range(256)]

def isbinary(s):
    """
    returns true iff s has binary characters: not alphanum and not "\n\r\t", etc.
    """
    for c in s:
        if isbinary_map[ord(c)]: return True
    return False
    
def isprintable(s):
    """
    true iff s only has printable characters: alphanum.
    """
    return len(repr(s)) == len(s) + 2

translate_char_to_print = ''.join([isprintable(chr(x)) and chr(x) or '.' for x in range(256)])

def hexdump(src, length=16):
    N = 0
    result = ''
    while src:
        s, src = src[:length], src[length:]
        hexa = ' '.join(["%02X"%ord(x) for x in s])
        s = s.translate(translate_char_to_print)
        result += "%04d   %-*s   %s\n" % (N, length*3, hexa, s)
        N += length
    return result

if __name__ == "__main__":
    s=("This 10 line function is just a sample of pyhton power "
       "for string manipulations.\n"
       "The code is \x07even\x08 quite readable!")
    print hexdump(s)
