///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.com)
//             2014-2019, PALANDesign Hannover, Germany
//
// \license The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// \brief Tiny printf, sprintf and (v)snprintf implementation, optimized for speed on
//        embedded systems with a very limited resources. These routines are thread
//        safe and reentrant!
//        Use this instead of the bloated standard/newlib printf cause these use
//        malloc for printf (and may not be thread safe).
//
///////////////////////////////////////////////////////////////////////////////

#include <stdbool.h>
#include <stdint.h>
#include <set>
#include <string>
#include <sstream>
#include "mcuprintf.h"
#include "encoding.h"

static inline bool _is_digit(char ch)
{
    return (ch >= '0') && (ch <= '9');
}

std::set<mf_char> str2set(const char *format)
{
    std::istringstream sstr(mcuprintf(format));
    std::set<mf_char> result;
    
    for (mf_char ch = mf_getchar(sstr); ch; ch = mf_getchar(sstr))
    {
        result.insert(ch);
    }
    return result;
}

// internal vsnprintf
std::string mcuprintf(const char *format)
{
    std::string binaryDigits("01"), octetDigits("01234567"), unsignedDigits("0123456789"), signedDigits(
            "0123456789-"), floatDigits("0123456789.-"), upperHexDigits("ABCDEF"), lowerHexDigits("abcdef");
    std::string result;
    
    size_t idx = 0U;
    int n;
    
    while (*format)
    {
        // format specifier?  %[flags][width][.precision][length]
        if (*format != '%')
        {
            // no
            // out(*format, buffer, idx++, maxlen);
            result.push_back(*format);
            format++;
            continue;
        }
        else
        {
            // yes, evaluate it
            format++;
        }
        
        // evaluate flags
        do
        {
            switch (*format)
            {
                case '0':
                    format++;
                    n = 1U;
                    break;
                case '-':
                    format++;
                    n = 1U;
                    break;
                case '+':
                    format++;
                    n = 1U;
                    break;
                case ' ':
                    format++;
                    n = 1U;
                    break;
                case '#':
                    format++;
                    n = 1U;
                    break;
                default :
                    n = 0U;
                    break;
            }
        } while (n);
        
        // evaluate width field
        if (_is_digit(*format))
        {
            while (_is_digit(*format))
                format++;
        }
        else if (*format == '*')
        {
            format++;
        }
        
        // evaluate precision field
        if (*format == '.')
        {
            format++;
            if (_is_digit(*format))
            {
                while (_is_digit(*format))
                    format++;
            }
            else if (*format == '*')
            {
                format++;
            }
        }
        
        // evaluate length field
        switch (*format)
        {
            case 'l' :
                format++;
                if (*format == 'l')
                {
                    format++;
                }
                break;
            case 'h' :
                format++;
                if (*format == 'h')
                {
                    format++;
                }
                break;
            case 'j' :
                format++;
                break;
            case 'z' :
                format++;
                break;
            default :
                break;
        }
        
        // evaluate specifier
        switch (*format)
        {
            case 'd' :
            case 'i' :
                result.append(signedDigits);
                format++;
                break;
            
            case 'u' :
                result.append(unsignedDigits);
                format++;
                break;
            
            case 'x' :
                result.append(unsignedDigits);
                result.append(lowerHexDigits);
                format++;
                break;
            
            case 'X' :
                result.append(unsignedDigits);
                result.append(upperHexDigits);
                format++;
                break;
            case 'o' :
                result.append(octetDigits);
                format++;
                break;
            
            case 'b' :
                result.append(binaryDigits);
                format++;
                break;
            case 'f' :
            case 'F' :
                result.append(floatDigits);
                format++;
                break;
                
            case 'p' :
                result.append(unsignedDigits);
                result.append(upperHexDigits);
                format++;
                break;
            
            case '%' :
                result.push_back('%');
                format++;
                break;
            
            default :
                format++;
                break;
        }
    }
    
    return result;
}


