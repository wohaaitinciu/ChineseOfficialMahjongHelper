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

#ifndef __MAHJONG_ALGORITHM__STRINGIFY_H__
#define __MAHJONG_ALGORITHM__STRINGIFY_H__

#include "tile.h"

namespace mahjong {

/**
 * @brief 字符串格式：
 * - 数牌：万=m 条=s 饼=p。后缀使用小写字母，一连串同花色的数牌可合并使用用一个后缀，如123m、678s等等。
 * - 字牌：东南西北=ESWN，中发白=CFP。使用大写字母。亦兼容天凤风格的后缀z，但按中国习惯顺序567z为中发白。
 * - 吃、碰、杠用英文[]，可选用逗号+数字表示供牌来源。数字的具体规则如下：
 *    - 吃：表示第几张牌是由上家打出，如[567m,2]表示57万吃6万（第2张）。对于不指定数字的，默认为吃第1张。
 *    - 碰：表示由哪家打出，1为上家，2为对家，3为下家，如[999s,3]表示碰下家的9条。对于不指定数字的，默认为碰上家。
 *    - 杠：与碰类似，但对于不指定数字的，则认为是暗杠。例如：[SSSS]表示暗杠南；[8888p,1]表示大明杠上家的8饼。当数字为5、6、7时，表示加杠。例如：[1111s,6]表示碰对家的1条后，又摸到1条加杠。
 * - 范例
 *    - [EEEE][CCCC][FFFF][PPPP]NN
 *    - 1112345678999s9s
 *    - [WWWW,1][444s]45m678pFF6m
 *    - [EEEE]288s349pSCFF2p
 *    - [123p,1][345s,2][999s,3]6m6pEW1m
 *    - 356m18s1579pWNFF9p
 */

/**
 * @addtogroup stringify
 * @{
 */

/**
 * @name error codes
 * @{
 *  解析牌的错误码
 */
#define PARSE_NO_ERROR 0                                ///< 无错误
#define PARSE_ERROR_ILLEGAL_CHARACTER -1                ///< 非法字符
#define PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT -2            ///< 数字后面缺少后缀
#define PARSE_ERROR_WRONG_TILES_COUNT_FOR_FIXED_PACK -3 ///< 副露包含错误的牌数目
#define PARSE_ERROR_CANNOT_MAKE_FIXED_PACK -4           ///< 无法正确解析副露
#define PARSE_ERROR_TOO_MANY_FIXED_PACKS -5             ///< 过多组副露（一副合法手牌最多4副露）
#define PARSE_ERROR_TOO_MANY_TILES -6                   ///< 过多牌
#define PARSE_ERROR_TILE_COUNT_GREATER_THAN_4 -7        ///< 某张牌出现超过4枚

 /**
  * @}
  */

/**
 * @brief 解析牌
 * @param [in] str 字符串
 * @param [out] tiles 牌
 * @param [in] max_cnt 牌的最大数量
 * @retval > 0 实际牌的数量
 * @retval == 0 失败
 */
intptr_t parse_tiles(const char *str, tile_t *tiles, intptr_t max_cnt);

/**
 * @brief 字符串转换为手牌结构和上牌
 * @param [in] str 字符串
 * @param [out] hand_tiles 手牌结构
 * @param [out] serving_tile 上的牌
 * @retval PARSE_NO_ERROR 无错误
 * @retval PARSE_ERROR_ILLEGAL_CHARACTER 非法字符
 * @retval PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT 数字后面缺少后缀
 * @retval PARSE_ERROR_WRONG_TILES_COUNT_FOR_FIXED_PACK 副露包含错误的牌数目
 * @retval PARSE_ERROR_CANNOT_MAKE_FIXED_PACK 无法正确解析副露
 * @retval PARSE_ERROR_TOO_MANY_FIXED_PACKS 过多组副露（一副合法手牌最多4副露）
 * @retval PARSE_ERROR_TOO_MANY_TILES 过多牌
 * @retval PARSE_ERROR_TILE_COUNT_GREATER_THAN_4 某张牌出现超过4枚
 */
intptr_t string_to_tiles(const char *str, hand_tiles_t *hand_tiles, tile_t *serving_tile);

/**
 * @brief 牌转换为字符串
 * @param [in] tiles 牌
 * @param [in] tile_cnt 牌的数量
 * @param [out] str 字符串
 * @param [in] max_size 字符串最大长度
 * @return intptr_t 写入的字符串数
 */
intptr_t tiles_to_string(const tile_t *tiles, intptr_t tile_cnt, char *str, intptr_t max_size);

/**
 * @brief 牌组转换为字符串
 * @param [in] packs 牌组
 * @param [in] pack_cnt 牌组的数量
 * @param [out] str 字符串
 * @param [in] max_size 字符串最大长度
 * @return intptr_t 写入的字符串数
 */
intptr_t packs_to_string(const pack_t *packs, intptr_t pack_cnt, char *str, intptr_t max_size);

/**
 * @brief 手牌结构转换为字符串
 * @param [in] hand_tiles 手牌结构
 * @param [out] str 字符串
 * @param [in] max_size 字符串最大长度
 * @return intptr_t 写入的字符串数
 */
intptr_t hand_tiles_to_string(const hand_tiles_t *hand_tiles, char *str, intptr_t max_size);

/**
 * end group
 * @}
 */

}

#endif
