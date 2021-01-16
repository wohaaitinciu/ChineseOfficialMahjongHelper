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

#ifndef __MAHJONG_ALGORITHM__SHANTEN_H__
#define __MAHJONG_ALGORITHM__SHANTEN_H__

#include "tile.h"

namespace mahjong {

/**
 * @brief 牌组转换成牌
 *
 * @param [in] packs 牌组
 * @param [in] pack_cnt 牌组的数量
 * @param [out] tiles 牌
 * @param [in] tile_cnt 牌的最大数量
 * @return intptr_t 牌的实际数量
 */
intptr_t packs_to_tiles(const pack_t *packs, intptr_t pack_cnt, tile_t *tiles, intptr_t tile_cnt);

/**
 * @brief 将牌打表
 *
 * @param [in] tiles 牌
 * @param [in] cnt 牌的数量
 * @param [out] cnt_table 牌的数量表
 */
void map_tiles(const tile_t *tiles, intptr_t cnt, tile_table_t *cnt_table);

/**
 * @brief 将手牌打表
 *
 * @param [in] hand_tiles 手牌
 * @param [out] cnt_table 牌的数量表
 * @return bool 手牌结构是否正确。即是否符合：副露组数*3+立牌数=13
 */
bool map_hand_tiles(const hand_tiles_t *hand_tiles, tile_table_t *cnt_table);

/**
 * @brief 将表转换成牌
 *
 * @param [in] cnt_table 牌的数量表
 * @param [out] tiles 牌
 * @param [in] max_cnt 牌的最大数量
 * @return intptr_t 牌的实际数量
 */
intptr_t table_to_tiles(const tile_table_t &cnt_table, tile_t *tiles, intptr_t max_cnt);

/**
 * @brief 有效牌标记表类型
 */
typedef bool useful_table_t[TILE_TABLE_SIZE];

/**
 * @addtogroup shanten
 * @{
 */

/**
 * @addtogroup basic_form
 * @{
 */

/**
 * @brief 基本和型上听数
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] useful_table 有效牌标记表（可为null）
 * @return int 上听数
 */
int basic_form_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table);

/**
 * @brief 基本和型是否听牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] waiting_table 听牌标记表（可为null）
 * @return bool 是否听牌
 */
bool is_basic_form_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table);

/**
 * @brief 基本和型是否和牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [in] test_tile 测试的牌
 * @return bool 是否和牌
 */
bool is_basic_form_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile);

/**
 * end group
 * @}
 */

/**
 * @addtogroup seven_pairs
 * @{
 */

/**
 * @brief 七对上听数
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] useful_table 有效牌标记表（可为null）
 * @return int 上听数
 */
int seven_pairs_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table);

/**
 * @brief 七对是否听牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] waiting_table 听牌标记表（可为null）
 * @return bool 是否听牌
 */
bool is_seven_pairs_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table);

/**
 * @brief 七对是否和牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [in] test_tile 测试的牌
 * @return bool 是否和牌
 */
bool is_seven_pairs_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile);

/**
 * end group
 * @}
 */

/**
 * @addtogroup thirteen_orphans
 * @{
 */

/**
 * @brief 十三幺上听数
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] useful_table 有效牌标记表（可为null）
 * @return int 上听数
 */
int thirteen_orphans_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table);

/**
 * @brief 十三幺是否听牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] waiting_table 听牌标记表（可为null）
 * @return bool 是否听牌
 */
bool is_thirteen_orphans_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table);

/**
 * @brief 十三幺是否和牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [in] test_tile 测试的牌
 * @return bool 是否和牌
 */
bool is_thirteen_orphans_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile);

/**
 * end group
 * @}
 */

/**
 * @addtogroup knitted_straight
 * @{
 */

/**
 * @brief 组合龙上听数
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] useful_table 有效牌标记表（可为null）
 * @return int 上听数
 */
int knitted_straight_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table);

/**
 * @brief 组合龙是否听牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] waiting_table 听牌标记表（可为null）
 * @return bool 是否听牌
 */
bool is_knitted_straight_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table);

/**
 * @brief 组合龙是否和牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [in] test_tile 测试的牌
 * @return bool 是否和牌
 */
bool is_knitted_straight_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile);

/**
 * end group
 * @}
 */

/**
 * @addtogroup honors_and_knitted_tiles
 * @{
 */

/**
 * @brief 全不靠上听数
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] useful_table 有效牌标记表（可为null）
 * @return int 上听数
 */
int honors_and_knitted_tiles_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table);

/**
 * @brief 全不靠是否听牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [out] waiting_table 听牌标记表（可为null）
 * @return bool 是否听牌
 */
bool is_honors_and_knitted_tiles_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table);

/**
 * @brief 全不靠是否和牌
 *
 * @param [in] standing_tiles 立牌
 * @param [in] standing_cnt 立牌数
 * @param [in] test_tile 测试的牌
 * @return bool 是否和牌
 */
bool is_honors_and_knitted_tiles_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile);

/**
 * end group
 * @}
 */

/**
 * @brief 是否听牌
 *
 * @param [in] hand_tiles 手牌结构
 * @param [out] useful_table 有效牌标记表（可为null）
 * @return bool 是否听牌
 */
bool is_waiting(const hand_tiles_t &hand_tiles, useful_table_t *useful_table);

/**
 * end group
 * @}
 */

/**
 * @name form flags
 * @{
 *  和型
 */
#define FORM_FLAG_BASIC_FORM                0x01  ///< 基本和型
#define FORM_FLAG_SEVEN_PAIRS               0x02  ///< 七对
#define FORM_FLAG_THIRTEEN_ORPHANS          0x04  ///< 十三幺
#define FORM_FLAG_HONORS_AND_KNITTED_TILES  0x08  ///< 全不靠
#define FORM_FLAG_KNITTED_STRAIGHT          0x10  ///< 组合龙
#define FORM_FLAG_ALL                       0xFF  ///< 全部和型
/**
 * @}
 */

/**
 * @brief 枚举打哪张牌的计算结果信息
 */
struct enum_result_t {
    tile_t discard_tile;                    ///< 打这张牌
    uint8_t form_flag;                      ///< 和牌形式
    int shanten;                            ///< 上听数
    useful_table_t useful_table;            ///< 有效牌标记表
};

/**
 * @brief 枚举打哪张牌的计算回调函数
 *
 * @param [in] context 从enum_discard_tile传过来的context原样传回
 * @param [in] result 计算结果
 * @retval true 继续枚举
 * @retval false 结束枚举
 */
typedef bool (*enum_callback_t)(void *context, const enum_result_t *result);

/**
 * @brief 枚举打哪张牌
 *
 * @param [in] hand_tiles 手牌结构
 * @param [in] serving_tile 上牌（可为0，此时仅计算手牌的信息）
 * @param [in] form_flag 计算哪些和型
 * @param [in] context 用户自定义参数，将原样从回调函数传回
 * @param [in] enum_callback 回调函数
 */
void enum_discard_tile(const hand_tiles_t *hand_tiles, tile_t serving_tile, uint8_t form_flag,
    void *context, enum_callback_t enum_callback);

}

/**
 * end group
 * @}
 */

#endif
