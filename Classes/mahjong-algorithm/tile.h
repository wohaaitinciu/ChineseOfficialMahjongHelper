/****************************************************************************
 Copyright (c) 2016-2021 Jeff Wang <summer_insects@163.com>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 ****************************************************************************/

#ifndef __MAHJONG_ALGORITHM__TILE_H__
#define __MAHJONG_ALGORITHM__TILE_H__

#include <stddef.h>
#include <stdint.h>

 // force inline
#ifndef FORCE_INLINE
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#define FORCE_INLINE __forceinline
#elif defined(__GNUC__) && ((__GNUC__ << 8 | __GNUC_MINOR__) >= 0x301)
#define FORCE_INLINE __inline__ __attribute__((__always_inline__))
#else
#define FORCE_INLINE inline
#endif
#endif

 // unreachable
#ifndef UNREACHABLE
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#define UNREACHABLE() __assume(0)
#elif defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ << 8 | __GNUC_MINOR__) >= 0x405))
#define UNREACHABLE() __builtin_unreachable()
#else
#define UNREACHABLE() assert(0)
#endif
#endif

namespace mahjong {

/**
 * @brief 代码注释中用到的术语简介
 * - 顺子：数牌中，花色相同序数相连的3张牌。
 * - 刻子：三张相同的牌。碰出的为明刻，未碰出的为暗刻。俗称坎。杠也算刻子，明杠算明刻，暗杠算暗刻。
 * - 面子：顺子和刻子的统称。俗称一句话、一坎牌。
 * - 雀头：基本和牌形式中，单独组合的对子，也叫将、眼。
 * - 基本和型：4面子1雀头的和牌形式。
 * - 特殊和型：非4面子1雀头的和牌形式，在国标规则中，有七对、十三幺、全不靠等特殊和型。
 * - 门清：也叫门前清，指不吃、不碰、不明杠的状态。特殊和型必然是门清状态。暗杠虽然不破门清，但会暴露出手牌不是特殊和型的信息。
 * - 副露：吃牌、碰牌、杠牌的统称，即利用其他选手打出的牌完成自己手牌面子的行为，一般不包括暗杠，也叫鸣牌，俗称动牌。
 *     副露有时候也包括暗杠，此时将暗杠称为之暗副露，而吃、碰、明杠称为明副露。
 * - 立牌：整个手牌除去吃、碰、杠之后的牌。
 * - 手牌：包括立牌和吃、碰、杠的牌，有时仅指立牌。
 * - 听牌：只差所需要的一张牌即能和牌的状态。俗称下叫、落叫、叫和（糊）。
 * - 一上听：指差一张就能听牌的状态，也叫一向听、一入听。以此类推有二上听、三上听、N上听。
 * - 上听数：达到听牌状态需要牌的张数。
 * - 有效牌：能使上听数减少的牌，也称进张牌、上张牌。
 * - 改良牌：能使有效牌增加的牌。通俗来说就是能使进张面变宽的牌。
 * - 对子：两张相同的牌。雀头一定是对子，但对子不一定是雀头。
 * - 两面：数牌中，花色相同数字相邻的两张牌，如45m，与两侧的牌都构成顺子。也叫两头。
 * - 嵌张：数牌中，花色相同数字相隔1的两张牌，如57s，只能与中间的牌构成顺子，中间的这张牌称为嵌张。
 * - 边张：也是数字相邻的两张牌，但由于处在边界位置，只能与一侧的牌能构成顺子，如12只能与3构成顺子、89只能与7构成顺子，这张3或者7便称为边张。
 * - 搭子：指差一张牌就能构成1组面子的两张牌。其形态有刻子搭子（即对子）、两面搭子、嵌张搭子、边张搭子。
 * - 复合搭子：多张牌构成的搭子。常见的有：连嵌张、两面带对子、嵌张带对子、边张带对子等等形态。
 * - 对倒：听牌时，其他牌都已经构成面子，剩余两对，只需任意一对成刻即可和牌，此时另一对充当雀头，这种听牌形态叫对倒，也叫双碰、对碰、对杵。
 */


/**
 * @addtogroup tile
 * @{
 */

/**
 * @brief 花色
 */
typedef uint8_t suit_t;

/**
 * @brief 点数
 */
typedef uint8_t rank_t;

#define TILE_SUIT_NONE          0  ///< 无效
#define TILE_SUIT_CHARACTERS    1  ///< 万子（CHARACTERS）
#define TILE_SUIT_BAMBOO        2  ///< 条子（BAMBOO）
#define TILE_SUIT_DOTS          3  ///< 饼子（DOTS）
#define TILE_SUIT_HONORS        4  ///< 字牌（HONORS）

/**
 * @brief 牌\n
 * 内存结构：
 * - 0-3 4bit 牌的点数
 * - 4-7 4bit 牌的花色
 * 合法的牌为：
 * - 0x11 - 0x19 万子（CHARACTERS）
 * - 0x21 - 0x29 条子（BAMBOO）
 * - 0x31 - 0x39 饼子（DOTS）
 * - 0x41 - 0x47 字牌（HONORS）
 * - 0x51 - 0x58 花牌（FLOWER）
 */
typedef uint8_t tile_t;

/**
 * @brief 生成一张牌
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] suit 花色
 * @param [in] rank 点数
 * @return tile_t 牌
 */
static FORCE_INLINE tile_t make_tile(suit_t suit, rank_t rank) {
    return (((suit & 0xF) << 4) | (rank & 0xF));
}

/**
 * @brief 获取牌的花色
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] tile 牌
 * @return suit_t 花色
 */
static FORCE_INLINE suit_t tile_get_suit(tile_t tile) {
    return ((tile >> 4) & 0xF);
}

/**
 * @brief 判断是否为花牌
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] tile 牌
 * @return bool
 */
static FORCE_INLINE bool is_flower(tile_t tile) {
    return ((tile >> 4) & 0xF) == 5;
}

/**
 * @brief 获取牌的点数
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] tile 牌
 * @return rank_t 点数
 */
static FORCE_INLINE rank_t tile_get_rank(tile_t tile) {
    return (tile & 0xF);
}

/**
 * @brief 所有牌的值，不包括花牌
 */
enum tile_value_t {
    TILE_1m = 0x11, TILE_2m, TILE_3m, TILE_4m, TILE_5m, TILE_6m, TILE_7m, TILE_8m, TILE_9m,
    TILE_1s = 0x21, TILE_2s, TILE_3s, TILE_4s, TILE_5s, TILE_6s, TILE_7s, TILE_8s, TILE_9s,
    TILE_1p = 0x31, TILE_2p, TILE_3p, TILE_4p, TILE_5p, TILE_6p, TILE_7p, TILE_8p, TILE_9p,
    TILE_E  = 0x41, TILE_S , TILE_W , TILE_N , TILE_C , TILE_F , TILE_P ,
    TILE_TABLE_SIZE
};

/**
 * @brief 所有合法的牌，不包括花牌
 */
static const tile_t all_tiles[] = {
    TILE_1m, TILE_2m, TILE_3m, TILE_4m, TILE_5m, TILE_6m, TILE_7m, TILE_8m, TILE_9m,
    TILE_1s, TILE_2s, TILE_3s, TILE_4s, TILE_5s, TILE_6s, TILE_7s, TILE_8s, TILE_9s,
    TILE_1p, TILE_2p, TILE_3p, TILE_4p, TILE_5p, TILE_6p, TILE_7p, TILE_8p, TILE_9p,
    TILE_E , TILE_S , TILE_W , TILE_N , TILE_C , TILE_F , TILE_P
};

/**
 * @brief 牌表类型
 *
 * 说明：在判断听牌、计算上听数等算法中，主流的对于牌有两种存储方式：
 * - 一种是用牌表，各索引表示各种牌拥有的枚数，这种存储方式的优点是在递归计算时削减面子只需要修改表中相应下标的值，缺点是一手牌的总数不方便确定
 * - 另一种是直接用牌的数组，这种存储方式的优点是很容易确定一手牌的总数，缺点是在递归计算时削减面子不方便，需要进行数组删除元素操作
 */
typedef uint16_t tile_table_t[TILE_TABLE_SIZE];

#define PACK_TYPE_NONE 0  ///< 无效
#define PACK_TYPE_CHOW 1  ///< 顺子
#define PACK_TYPE_PUNG 2  ///< 刻子
#define PACK_TYPE_KONG 3  ///< 杠
#define PACK_TYPE_PAIR 4  ///< 雀头

/**
 * @brief 牌组
 *  用于表示一组面子或者雀头
 *
 * 内存结构：
 * - 0-7 8bit tile 牌（对于顺子，则表示中间那张牌，比如234p，那么牌为3p）
 * - 8-11 4bit type 牌组类型，使用PACK_TYPE_xxx宏
 * - 12-13 2bit offer 供牌信息，取值范围为0123\n
 * - 14 1bit promoted 是否为加杠
 *       0表示暗手（暗顺、暗刻、暗杠），非0表示明手（明顺、明刻、明杠）
 *
 *       对于牌组是刻子和杠时，123分别来表示是上家/对家/下家供的\n
 *       对于牌组为顺子时，由于吃牌只能是上家供，这里用123分别来表示第几张是上家供的
 */
typedef uint16_t pack_t;

/**
 * @brief 生成一个牌组
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] offer 供牌信息
 * @param [in] type 牌组类型
 * @param [in] tile 牌（对于顺子，为中间那张牌）
 */
static FORCE_INLINE pack_t make_pack(uint8_t offer, uint8_t type, tile_t tile) {
    return (offer << 12 | (type << 8) | tile);
}

/**
 * @brief 牌组是否为明的
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] pack 牌组
 * @return bool
 */
static FORCE_INLINE bool is_pack_melded(pack_t pack) {
    return !!(pack & 0x3000);
}

/**
 * @brief 牌组是否为加杠
 *  当牌组不是PACK_TYPE_KONG时，结果是无意义的
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] pack 牌组
 * @return bool
 */
static FORCE_INLINE bool is_promoted_kong(pack_t pack) {
    return !!(pack & 0x4000);
}

/**
 * @brief 碰的牌组转换为加杠
 *  当牌组不是PACK_TYPE_PUNG时，结果是无意义的
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] pack 碰的牌组
 * @return pack_t 加杠的牌组
 */
static FORCE_INLINE pack_t promote_pung_to_kong(pack_t pack) {
    return pack | 0x4300;
}

/**
 * @brief 牌组的供牌信息
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] pack 牌组
 * @return uint8_t
 */
static FORCE_INLINE uint8_t pack_get_offer(pack_t pack) {
    return ((pack >> 12) & 0x3);
}

/**
 * @brief 获取牌组的类型
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] pack 牌组
 * @return uint8_t 牌组类型
 */
static FORCE_INLINE uint8_t pack_get_type(pack_t pack) {
    return ((pack >> 8) & 0xF);
}

/**
 * @brief 获取牌的点数
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] pack 牌组
 * @return tile_t 牌（对于顺子，为中间那张牌）
 */
static FORCE_INLINE tile_t pack_get_tile(pack_t pack) {
    return (pack & 0xFF);
}

/**
 * @brief 手牌结构
 *  手牌结构一定满足等式：3*副露的牌组数+立牌数=13
 */
struct hand_tiles_t {
    pack_t fixed_packs[5];      ///< 副露的牌组（面子），包括暗杠
    intptr_t pack_count;        ///< 副露的牌组（面子）数，包括暗杠
    tile_t standing_tiles[13];  ///< 立牌
    intptr_t tile_count;        ///< 立牌数
};


/**
 * @brief 判断是否为绿一色构成牌
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] tile 牌
 * @return bool
 */
static FORCE_INLINE bool is_green(tile_t tile) {
    // 最基本的逐个判断，23468s及发财为绿一色构成牌
    //return (tile == TILE_2s || tile == TILE_3s || tile == TILE_4s || tile == TILE_6s || tile == TILE_8s || tile == TILE_F);

    // 算法原理：
    // 0x48-0x11=0x37=55刚好在一个64位整型的范围内，
    // 用uint64_t的每一位表示一张牌的标记，事先得到一个魔数，
    // 然后每次测试相应位即可
    return !!(0x0020000000AE0000ULL & (1ULL << (tile - TILE_1m)));
}

/**
 * @brief 判断是否为推不倒构成牌
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] tile 牌
 * @return bool
 */
static FORCE_INLINE bool is_reversible(tile_t tile) {
    // 最基本的逐个判断：245689s、1234589p及白板为推不倒构成牌
    //return (tile == TILE_2s || tile == TILE_4s || tile == TILE_5s || tile == TILE_6s || tile == TILE_8s || tile == TILE_9s ||
    //    tile == TILE_1p || tile == TILE_2p || tile == TILE_3p || tile == TILE_4p || tile == TILE_5p || tile == TILE_8p || tile == TILE_9p ||
    //    tile == TILE_P);

    // 算法原理同绿一色构成牌判断函数
    return !!(0x0040019F01BA0000ULL & (1ULL << (tile - TILE_1m)));
}

/**
 * @brief 判断是否为数牌幺九（老头牌）
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] tile 牌
 * @return bool
 */
static FORCE_INLINE bool is_terminal(tile_t tile) {
    // 最基本的逐个判断
    //return (tile == TILE_1m || tile == TILE_9m || tile == TILE_1s || tile == TILE_9s || tile == TILE_1p || tile == TILE_9p);

    // 算法原理：观察数牌幺九的二进制位：
    // 0x11：0001 0001
    // 0x19：0001 1001
    // 0x21：0010 0001
    // 0x29：0010 1001
    // 0x31：0011 0001
    // 0x39：0011 1001
    // 所有牌的低4bit只会出现在0001到1001之间，跟0111位与，只有0001和1001的结果为1
    // 所有数牌的高4bit只会出现在0001到0011之间，跟1100位与，必然为0
    // 于是构造魔数0xC7（1100 0111）跟牌位与，结果为1的，就为数牌幺九
    // 缺陷：低4bit的操作会对0xB、0xD、0xF产生误判，高4bit的操作会对0x01和0x09产生误判
    return ((tile & 0xC7) == 1);
}

/**
 * @brief 判断是否为风牌
 * @param [in] tile 牌
 * @return bool
 */
static FORCE_INLINE bool is_winds(tile_t tile) {
    return (tile > 0x40 && tile < 0x45);
}

/**
 * @brief 判断是否为箭牌（三元牌）
 * @param [in] tile 牌
 * @return bool
 */
static FORCE_INLINE bool is_dragons(tile_t tile) {
    return (tile > 0x44 && tile < 0x48);
}

/**
 * @brief 判断是否为字牌
 * @param [in] tile 牌
 * @return bool
 */
static FORCE_INLINE bool is_honor(tile_t tile) {
    return (tile > 0x40 && tile < 0x48);
}

/**
 * @brief 判断是否为数牌
 * @param [in] tile 牌
 * @return bool
 */
static FORCE_INLINE bool is_numbered_suit(tile_t tile) {
    if (tile < 0x1A) return (tile > 0x10);
    if (tile < 0x2A) return (tile > 0x20);
    if (tile < 0x3A) return (tile > 0x30);
    return false;
}

/**
 * @brief 判断是否为数牌（更快）
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @see is_numbered_suit
 * @param [in] tile 牌
 * @return bool
 */
static FORCE_INLINE bool is_numbered_suit_quick(tile_t tile) {
    // 算法原理：数牌为0x11-0x19，0x21-0x29，0x31-0x39，跟0xC0位与，结果为0
    return !(tile & 0xC0);
}

/**
 * @brief 判断是否为幺九牌（包括数牌幺九和字牌）
 * @param [in] tile 牌
 * @return bool
 */
static FORCE_INLINE bool is_terminal_or_honor(tile_t tile) {
    return is_terminal(tile) || is_honor(tile);
}

/**
 * @brief 判断两张牌花色是否相同（更快）
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] tile0 牌0
 * @param [in] tile1 牌1
 * @return bool
 */
static FORCE_INLINE bool is_suit_equal_quick(tile_t tile0, tile_t tile1) {
    // 算法原理：高4bit表示花色
    return ((tile0 & 0xF0) == (tile1 & 0xF0));
}

/**
 * @brief 判断两张牌点数是否相同（更快）
 *  函数不检查输入的合法性。如果输入不合法的值，将无法保证合法返回值的合法性
 * @param [in] tile0 牌0
 * @param [in] tile1 牌1
 * @return bool
 */
static FORCE_INLINE bool is_rank_equal_quick(tile_t tile0, tile_t tile1) {
    // 算法原理：低4bit表示花色。高4bit设置为C是为了过滤掉字牌
    return ((tile0 & 0xCF) == (tile1 & 0xCF));
}

/**
 * end group
 * @}
 */

}

#endif
