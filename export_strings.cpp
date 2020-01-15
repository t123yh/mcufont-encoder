//
// Created by t123yh on 2020/1/10.
//

#include <vector>
#include "export_strings.h"

/* Header format:
 * lower 4 bits: Font ID
 * bit 4: Whether this is a non-languaged string
 * bit 5: Render option: middle
 * bit 6: Render option: erase
 * bit 7: Reserved
 */

static uint8_t create_header(uint8_t fontId, bool isNonLanguaged, bool isMiddle, bool isErase)
{
    return (fontId & 0xF) | (isNonLanguaged ? (1 << 4) : 0) | (isMiddle ? (1 << 5) : 0) | (isErase ? (1 << 6) : 0);
}

void export_strings(std::vector<UIString> &src, std::ostream & oss)
{
    std::vector<uint8_t> buf;
    for (auto &str : src)
    {
        str.Pos = buf.size();
        if (str.Default) // non-languaged
        {
            buf.push_back(create_header(str.Default->Font->Id, true, str.Middle, str.Erase));
            for (char ch : str.Default->Value)
            {
                buf.push_back((uint8_t) ch);
            }
            buf.push_back(0);
        }
        else
        {
            for (const auto &slang : str.Langs)
            {
                buf.push_back(create_header(slang.second.Font->Id, false, str.Middle, str.Erase));
                for (char ch : slang.second.Value)
                {
                    buf.push_back((uint8_t) ch);
                }
                buf.push_back(0);
            }
        }
    }
    oss.write(reinterpret_cast<const char*>(&buf[0]), buf.size());
}
