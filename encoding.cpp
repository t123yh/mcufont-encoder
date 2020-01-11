//
// Created by t123yh on 2019/12/8.
//

#include <cstdint>
#include <encoding.h>
#include <iostream>

mf_char mf_getchar(std::istream& inp)
{
    uint8_t c;
    uint8_t tmp, seqlen;
    uint16_t result;
    
    c = inp.get();
    if (inp.eof())
        return 0;
    
    if ((c & 0x80) == 0)
    {
        /* Just normal ASCII character. */
        return c;
    }
    else if ((c & 0xC0) == 0x80)
    {
        /* Dangling piece of corrupted multibyte sequence.
         * Did you cut the string in the wrong place?
         */
        return c;
    }
    else if ((inp.peek() & 0xC0) == 0xC0)
    {
        /* Start of multibyte sequence without any following bytes.
         * Silly. Maybe you are using the wrong encoding.
         */
        return c;
    }
    else
    {
        /* Beginning of a multi-byte sequence.
         * Find out how many characters and combine them.
         */
        seqlen = 2;
        tmp = 0x20;
        result = 0;
        while ((c & tmp) && (seqlen < 5))
        {
            seqlen++;
            tmp >>= 1;
            
            result = (result << 6) | (inp.peek() & 0x3F);
            inp.get();
        }
        
        result = (result << 6) | (inp.peek() & 0x3F);
        inp.get();
        
        result |= (c & (tmp - 1)) << ((seqlen - 1) * 6);
        return result;
    }
}

