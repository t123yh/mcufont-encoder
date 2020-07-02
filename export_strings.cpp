//
// Created by t123yh on 2020/1/10.
//

#include <vector>
#include "export_strings.h"

/* Header format:
 * lower 4 bits: Font ID
 * bit 4: Whether this is a non-languaged string
 * bit [6:5]: Render option: align where 00 = left, 01 = middle, 10 = right
 * bit 7: Render option: erase
 */

static uint8_t create_header(uint8_t fontId, bool isNonLanguaged, UIString::AlignType isMiddle, bool isErase)
{
    if (fontId > 0xF) {
        throw std::runtime_error("Too much font, maximum 15");
    }
    return (fontId & 0xF) | (isNonLanguaged ? (1 << 4) : 0) | (((uint8_t)isMiddle) << 5U) | (isErase ? (1 << 7) : 0);
}

void export_strings(std::vector<UIString> &src, std::ostream & oss)
{
    std::vector<uint8_t> buf;
    buf.push_back(0); // skip the first byte to skip position 0 (pos 0 means empty string)
    for (auto &str : src)
    {
        str.Pos = buf.size();
        if (str.Default) // non-languaged
        {
            buf.push_back(create_header(str.Default->Font->Id, true, str.Align, str.Erase));
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
                buf.push_back(create_header(slang.second.Font->Id, false, str.Align, str.Erase));
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
