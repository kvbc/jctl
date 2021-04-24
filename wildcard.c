/*
 * Wildcard matching engine taken from the github repository of PuTTY,
 * a free Windows and Unix Telnet and SSH client.
 * https://github.com/github/putty
 */

/*
 * PuTTY is copyright 1997-2020 Simon Tatham.
 * 
 * Portions copyright Robert de Bath, Joris van Rantwijk, Delian
 * Delchev, Andreas Schultz, Jeroen Massar, Wez Furlong, Nicolas Barry,
 * Justin Bradford, Ben Harris, Malcolm Smith, Ahmad Khalifa, Markus
 * Kuhn, Colin Watson, Christopher Staite, Lorenz Diener, Christian
 * Brabandt, Jeff Smith, Pavel Kryukov, Maxim Kuznetsov, Svyatoslav
 * Kuzmich, Nico Williams, Viktor Dukhovni, Josh Dersch, Lars Brinkhoff,
 * and CORE SDI S.A.
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "wildcard.h"
#include "jctl.h"

/*
 * Definition of wildcard syntax:
 *
 *  - * matches any sequence of characters, including zero.
 *  - ? matches exactly one character which can be anything.
 *  - [abc] matches exactly one character which is a, b or c.
 *  - [a-f] matches anything from a through f.
 *  - [^a-f] matches anything _except_ a through f.
 *  - [-_] matches - or _; [^-_] matches anything else. (The - is
 *    non-special if it occurs immediately after the opening
 *    bracket or ^.)
 *  - [a^] matches an a or a ^. (The ^ is non-special if it does
 *    _not_ occur immediately after the opening bracket.)
 *  - \*, \?, \[, \], \\ match the single characters *, ?, [, ], \.
 *  - All other characters are non-special and match themselves.
 */

/*
 * Some notes on differences from POSIX globs (IEEE Std 1003.1, 2003 ed.):
 *  - backslashes act as escapes even within [] bracket expressions
 *  - does not support [!...] for non-matching list (POSIX are weird);
 *    NB POSIX allows [^...] as well via "A bracket expression starting
 *    with an unquoted circumflex character produces unspecified
 *    results". If we wanted to allow [!...] we might want to define
 *    [^!] as having its literal meaning (match '^' or '!').
 *  - none of the scary [[:class:]] stuff, etc
 */

/*
 * The wildcard matching technique we use is very simple and
 * potentially O(N^2) in running time, but I don't anticipate it
 * being that bad in reality (particularly since N will be the size
 * of a filename, which isn't all that much). Perhaps one day, once
 * PuTTY has grown a regexp matcher for some other reason, I might
 * come back and reimplement wildcards by translating them into
 * regexps or directly into NFAs; but for the moment, in the
 * absence of any other need for the NFA->DFA translation engine,
 * anything more than the simplest possible wildcard matcher is
 * vast code-size overkill.
 *
 * Essentially, these wildcards are much simpler than regexps in
 * that they consist of a sequence of rigid fragments (? and [...]
 * can never match more or less than one character) separated by
 * asterisks. It is therefore extremely simple to look at a rigid
 * fragment and determine whether or not it begins at a particular
 * point in the test string; so we can search along the string
 * until we find each fragment, then search for the next. As long
 * as we find each fragment in the _first_ place it occurs, there
 * will never be a danger of having to backpedal and try to find it
 * again somewhere else.
 */


 enum
 {
    WC_TRAILINGBACKSLASH = 1,
    WC_UNCLOSEDCLASS,
    WC_INVALIDRANGE
};


/*
 * This is the routine that tests a target string to see if an
 * initial substring of it matches a fragment. If successful, it
 * returns 1, and advances both `fragment' and `target' past the
 * fragment and matching substring respectively. If unsuccessful it
 * returns zero. If the wildcard fragment suffers a syntax error,
 * it returns <0 and the precise value indexes into wc_error.
 */
static int wc_match_fragment(const char **fragment, const char **target,
                             const char *target_end)
{
    const char *f, *t;

    f = *fragment;
    t = *target;
    /*
     * The fragment terminates at either the end of the string, or
     * the first (unescaped) *.
     */
    while (*f && *f != '*' && t < target_end) {
        /*
         * Extract one character from t, and one character's worth
         * of pattern from f, and step along both. Return 0 if they
         * fail to match.
         */
        if (*f == '\\') {
            /*
             * Backslash, which means f[1] is to be treated as a
             * literal character no matter what it is. It may not
             * be the end of the string.
             */
            if (!f[1])
                return -WC_TRAILINGBACKSLASH;   /* error */
            if (f[1] != *t)
                return 0;              /* failed to match */
            f += 2;
        } else if (*f == '?') {
            /*
             * Question mark matches anything.
             */
            f++;
        } else if (*f == '[') {
            int invert = 0;
            int matched = 0;
            /*
             * Open bracket introduces a character class.
             */
            f++;
            if (*f == '^') {
                invert = 1;
                f++;
            }
            while (*f != ']') {
                if (*f == '\\')
                    f++;               /* backslashes still work */
                if (!*f)
                    return -WC_UNCLOSEDCLASS;   /* error again */
                if (f[1] == '-') {
                    int lower, upper, ourchr;
                    lower = (unsigned char) *f++;
                    f++;               /* eat the minus */
                    if (*f == ']')
                        return -WC_INVALIDRANGE;   /* different error! */
                    if (*f == '\\')
                        f++;           /* backslashes _still_ work */
                    if (!*f)
                        return -WC_UNCLOSEDCLASS;   /* error again */
                    upper = (unsigned char) *f++;
                    ourchr = (unsigned char) *t;
                    if (lower > upper) {
                        int t = lower; lower = upper; upper = t;
                    }
                    if (ourchr >= lower && ourchr <= upper)
                        matched = 1;
                } else {
                    matched |= (*t == *f++);
                }
            }
            if (invert == matched)
                return 0;              /* failed to match character class */
            f++;                       /* eat the ] */
        } else {
            /*
             * Non-special character matches itself.
             */
            if (*f != *t)
                return 0;
            f++;
        }
        /*
         * Now we've done that, increment t past the character we
         * matched.
         */
        t++;
    }
    if (!*f || *f == '*') {
        /*
         * We have reached the end of f without finding a mismatch;
         * so we're done. Update the caller pointers and return 1.
         */
        *fragment = f;
        *target = t;
        return 1;
    }
    /*
     * Otherwise, we must have reached the end of t before we
     * reached the end of f; so we've failed. Return 0.
     */
    return 0;
}

/*
 * This is the real wildcard matching routine. It returns 1 for a
 * successful match, 0 for an unsuccessful match, and <0 for a
 * syntax error in the wildcard.
 */
int wc_match(
    const char *wildcard, const char *target, size_t target_len)
{
    const char *target_end = target + target_len;
    int ret;

    /*
     * Every time we see a '*' _followed_ by a fragment, we just
     * search along the string for a location at which the fragment
     * matches. The only special case is when we see a fragment
     * right at the start, in which case we just call the matching
     * routine once and give up if it fails.
     */
    if (*wildcard != '*') {
        ret = wc_match_fragment(&wildcard, &target, target_end);
        if (ret <= 0)
            return ret;                /* pass back failure or error alike */
    }

    while (*wildcard) {
        assert(*wildcard == '*');
        while (*wildcard == '*')
            wildcard++;

        /*
         * It's possible we've just hit the end of the wildcard
         * after seeing a *, in which case there's no need to
         * bother searching any more because we've won.
         */
        if (!*wildcard)
            return 1;

        /*
         * Now `wildcard' points at the next fragment. So we
         * attempt to match it against `target', and if that fails
         * we increment `target' and try again, and so on. When we
         * find we're about to try matching against the empty
         * string, we give up and return 0.
         */
        ret = 0;
        while (*target) {
            const char *save_w = wildcard, *save_t = target;

            ret = wc_match_fragment(&wildcard, &target, target_end);

            if (ret < 0)
                return ret;            /* syntax error */

            if (ret > 0 && !*wildcard && target != target_end) {
                /*
                 * Final special case - literally.
                 *
                 * This situation arises when we are matching a
                 * _terminal_ fragment of the wildcard (that is,
                 * there is nothing after it, e.g. "*a"), and it
                 * has matched _too early_. For example, matching
                 * "*a" against "parka" will match the "a" fragment
                 * against the _first_ a, and then (if it weren't
                 * for this special case) matching would fail
                 * because we're at the end of the wildcard but not
                 * at the end of the target string.
                 *
                 * In this case what we must do is measure the
                 * length of the fragment in the target (which is
                 * why we saved `target'), jump straight to that
                 * distance from the end of the string using
                 * strlen, and match the same fragment again there
                 * (which is why we saved `wildcard'). Then we
                 * return whatever that operation returns.
                 */
                target = target_end - (target - save_t);
                wildcard = save_w;
                return wc_match_fragment(&wildcard, &target, target_end);
            }

            if (ret > 0)
                break;
            target++;
        }
        if (ret > 0)
            continue;
        return 0;
    }

    /*
     * If we reach here, it must be because we successfully matched
     * a fragment and then found ourselves right at the end of the
     * wildcard. Hence, we return 1 if and only if we are also
     * right at the end of the target.
     */
    return target == target_end;
}


/*
 * Check if the given string 'str' matches the wildcard syntax.
 * Return 1 if it does, otherwise return 0.
 */
int wc_correct (const char *str)
{
    return _jctl_strspn("*^[]", str) > 0;
}
