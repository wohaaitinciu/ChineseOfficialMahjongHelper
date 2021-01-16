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

#include "shanten.h"
#include <assert.h>
#include <string.h>
#include <limits>
#include <algorithm>
#include <iterator>
#include "standard_tiles.h"

namespace mahjong {

// 牌组转换成牌
intptr_t packs_to_tiles(const pack_t *packs, intptr_t pack_cnt, tile_t *tiles, intptr_t tile_cnt) {
    if (packs == nullptr || tiles == nullptr) {
        return 0;
    }

    intptr_t cnt = 0;
    for (int i = 0; i < pack_cnt && cnt < tile_cnt; ++i) {
        tile_t tile = pack_get_tile(packs[i]);
        switch (pack_get_type(packs[i])) {
        case PACK_TYPE_CHOW:
            if (cnt < tile_cnt) tiles[cnt++] = static_cast<tile_t>(tile - 1);
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = static_cast<tile_t>(tile + 1);
            break;
        case PACK_TYPE_PUNG:
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            break;
        case PACK_TYPE_KONG:
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            break;
        case PACK_TYPE_PAIR:
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            break;
        default:
            UNREACHABLE();
            break;
        }
    }
    return cnt;
}

// 将牌打表
void map_tiles(const tile_t *tiles, intptr_t cnt, tile_table_t *cnt_table) {
    memset(*cnt_table, 0, sizeof(*cnt_table));
    for (intptr_t i = 0; i < cnt; ++i) {
        ++(*cnt_table)[tiles[i]];
    }
}

// 将手牌打表
bool map_hand_tiles(const hand_tiles_t *hand_tiles, tile_table_t *cnt_table) {
    // 将每一组副露当作3张牌来算，那么总张数=13
    if (hand_tiles->tile_count <= 0 || hand_tiles->pack_count < 0 || hand_tiles->pack_count > 4
        || hand_tiles->pack_count * 3 + hand_tiles->tile_count != 13) {
        return false;
    }

    // 将副露恢复成牌
    tile_t tiles[18];
    intptr_t tile_cnt = 0;
    if (hand_tiles->pack_count == 0) {
        memcpy(tiles, hand_tiles->standing_tiles, 13 * sizeof(tile_t));
        tile_cnt = 13;
    }
    else {
        tile_cnt = packs_to_tiles(hand_tiles->fixed_packs, hand_tiles->pack_count, tiles, 18);
        memcpy(tiles + tile_cnt, hand_tiles->standing_tiles, hand_tiles->tile_count * sizeof(tile_t));
        tile_cnt += hand_tiles->tile_count;
    }

    // 打表
    map_tiles(tiles, tile_cnt, cnt_table);
    return true;
}

// 将表转换成牌
intptr_t table_to_tiles(const tile_table_t &cnt_table, tile_t *tiles, intptr_t max_cnt) {
    intptr_t cnt = 0;
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        for (int n = 0; n < cnt_table[t]; ++n) {
            *tiles++ = t;
            ++cnt;
            if (cnt == max_cnt) {
                return max_cnt;
            }
        }
    }
    return cnt;
}

namespace {

    // 路径单元，单元有面子、雀头、搭子等种类，见下面的宏
    // 高8位表示类型，低8位表示牌
    // 对于顺子和顺子搭子，牌指的是最小的一张牌，
    // 例如在顺子123万中，牌为1万，在两面搭子45条中，牌为4条等等
    typedef uint16_t path_unit_t;

#define UNIT_TYPE_CHOW 1                // 顺子
#define UNIT_TYPE_PUNG 2                // 刻子
#define UNIT_TYPE_PAIR 4                // 雀头
#define UNIT_TYPE_CHOW_OPEN_END 5       // 两面或者边张搭子
#define UNIT_TYPE_CHOW_CLOSED 6         // 嵌张搭子
#define UNIT_TYPE_INCOMPLETE_PUNG 7     // 刻子搭子

#define MAKE_UNIT(type_, tile_) static_cast<path_unit_t>(((type_) << 8) | (tile_))
#define UNIT_TYPE(unit_) (((unit_) >> 8) & 0xFF)
#define UNIT_TILE(unit_) ((unit_) & 0xFF)

#define MAX_STATE 512
#define UNIT_SIZE 7

    // 一条路径
    struct work_path_t {
        path_unit_t units[UNIT_SIZE];  // 14/2=7最多7个搭子
        uint16_t depth;  // 当前路径深度
    };

    // 当前工作状态
    struct work_state_t {
        work_path_t paths[MAX_STATE];  // 所有路径
        intptr_t count;  // 路径数量
    };
}

// 路径是否来过了
static bool is_basic_form_branch_exist(const intptr_t fixed_cnt, const work_path_t *work_path, const work_state_t *work_state) {
    if (work_state->count <= 0 || work_path->depth == 0) {
        return false;
    }

    // depth处有信息，所以按stl风格的end应该要+1
    const uint16_t depth = static_cast<uint16_t>(work_path->depth + 1);

    // std::includes要求有序，但又不能破坏当前数据
    work_path_t temp;
    std::copy(&work_path->units[fixed_cnt], &work_path->units[depth], &temp.units[fixed_cnt]);
    std::sort(&temp.units[fixed_cnt], &temp.units[depth]);

    return std::any_of(&work_state->paths[0], &work_state->paths[work_state->count],
        [&temp, fixed_cnt, depth](const work_path_t &path) {
        return std::includes(&path.units[fixed_cnt], &path.units[path.depth], &temp.units[fixed_cnt], &temp.units[depth]);
    });
}

// 保存路径
static void save_work_path(const intptr_t fixed_cnt, const work_path_t *work_path, work_state_t *work_state) {
    // 复制一份数据，不破坏当前数据
    work_path_t temp;
    temp.depth = work_path->depth;
    std::copy(&work_path->units[fixed_cnt], &work_path->units[temp.depth + 1], &temp.units[fixed_cnt]);
    std::sort(&temp.units[fixed_cnt], &temp.units[temp.depth + 1]);

    // 判断是否重复
    if (std::none_of(&work_state->paths[0], &work_state->paths[work_state->count],
        [&temp, fixed_cnt](const work_path_t &path) {
        return (path.depth == temp.depth && std::equal(&path.units[fixed_cnt], &path.units[path.depth + 1], &temp.units[fixed_cnt]));
    })) {
        if (work_state->count < MAX_STATE) {
            work_path_t &path = work_state->paths[work_state->count++];
            path.depth = temp.depth;
            std::copy(&temp.units[fixed_cnt], &temp.units[temp.depth + 1], &path.units[fixed_cnt]);
        }
        else {
            assert(0 && "too many state!");
        }
    }
}

// 递归计算基本和型上听数
// 参数说明：
//   cnt_table牌表
//   has_pair是否有雀头
//   pack_cnt完成的面子数
//   incomplete_cnt搭子数
// 最后三个参数为优化性能用的，
// work_path保存当前正在计算的路径，
// work_state保存了所有已经计算过的路径，
// 从0到fixed_cnt的数据是不使用的，这些保留给了副露的面子
static int basic_form_shanten_recursively(tile_table_t &cnt_table, const bool has_pair, const unsigned pack_cnt, const unsigned incomplete_cnt,
    const intptr_t fixed_cnt, work_path_t *work_path, work_state_t *work_state) {
    if (fixed_cnt == 4) {  // 4副露
        for (int i = 0; i < 34; ++i) {
            tile_t t = all_tiles[i];
            if (cnt_table[t] > 1) {
                return -1;
            }
        }
        return 0;
    }

    if (pack_cnt == 4) {  // 已经有4组面子
        return has_pair ? -1 : 0;  // 如果有雀头，则和了；如果无雀头，则是听牌
    }

    int max_ret;  // 当前状态能返回的最大上听数

    // 算法说明：
    // 缺少的面子数=4-完成的面子数
    // 缺少的搭子数=缺少的面子数-已有的搭子数
    // 两式合并：缺少的搭子数=4-完成的面子数-已有的搭子数
    int incomplete_need = 4 - pack_cnt - incomplete_cnt;
    if (incomplete_need > 0) {  // 还需要搭子的情况
        // 有雀头时，上听数=已有的搭子数+缺少的搭子数*2-1
        // 无雀头时，上听数=已有的搭子数+缺少的搭子数*2
        max_ret = incomplete_cnt + incomplete_need * 2 - (has_pair ? 1 : 0);
    }
    else {  // 搭子齐了的情况
        // 有雀头时，上听数=3-完成的面子数
        // 无雀头时，上听数=4-完成的面子数
        max_ret = (has_pair ? 3 : 4) - pack_cnt;
    }

    // 当前路径深度
    const unsigned depth = pack_cnt + incomplete_cnt + has_pair;
    work_path->depth = static_cast<uint16_t>(depth);

    int result = max_ret;

    if (pack_cnt + incomplete_cnt > 4) {  // 搭子超载
        save_work_path(fixed_cnt, work_path, work_state);
        return max_ret;
    }

    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 1) {
            continue;
        }

        // 雀头
        if (!has_pair && cnt_table[t] > 1) {
            work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_PAIR, t);  // 记录雀头
            if (!is_basic_form_branch_exist(fixed_cnt, work_path, work_state)) {
                // 削减雀头，递归
                cnt_table[t] -= 2;
                int ret = basic_form_shanten_recursively(cnt_table, true, pack_cnt, incomplete_cnt,
                    fixed_cnt, work_path, work_state);
                result = std::min(ret, result);
                // 还原
                cnt_table[t] += 2;
            }
        }

        // 刻子
        if (cnt_table[t] > 2) {
            work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_PUNG, t);  // 记录刻子
            if (!is_basic_form_branch_exist(fixed_cnt, work_path, work_state)) {
                // 削减这组刻子，递归
                cnt_table[t] -= 3;
                int ret = basic_form_shanten_recursively(cnt_table, has_pair, pack_cnt + 1, incomplete_cnt,
                    fixed_cnt, work_path, work_state);
                result = std::min(ret, result);
                // 还原
                cnt_table[t] += 3;
            }
        }

        // 顺子（只能是数牌）
        bool is_numbered = is_numbered_suit(t);
        // 顺子t t+1 t+2，显然t不能是8点以上的数牌
        if (is_numbered && tile_get_rank(t) < 8 && cnt_table[t + 1] && cnt_table[t + 2]) {
            work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_CHOW, t);  // 记录顺子
            if (!is_basic_form_branch_exist(fixed_cnt, work_path, work_state)) {
                // 削减这组顺子，递归
                --cnt_table[t];
                --cnt_table[t + 1];
                --cnt_table[t + 2];
                int ret = basic_form_shanten_recursively(cnt_table, has_pair, pack_cnt + 1, incomplete_cnt,
                    fixed_cnt, work_path, work_state);
                result = std::min(ret, result);
                // 还原
                ++cnt_table[t];
                ++cnt_table[t + 1];
                ++cnt_table[t + 2];
            }
        }

        // 如果已经通过削减雀头/面子降低了上听数，再按搭子计算的上听数肯定不会更少
        if (result < max_ret) {
            continue;
        }

        // 刻子搭子
        if (cnt_table[t] > 1) {
            work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_INCOMPLETE_PUNG, t);  // 记录刻子搭子
            if (!is_basic_form_branch_exist(fixed_cnt, work_path, work_state)) {
                // 削减刻子搭子，递归
                cnt_table[t] -= 2;
                int ret = basic_form_shanten_recursively(cnt_table, has_pair, pack_cnt, incomplete_cnt + 1,
                    fixed_cnt, work_path, work_state);
                result = std::min(ret, result);
                // 还原
                cnt_table[t] += 2;
            }
        }

        // 顺子搭子（只能是数牌）
        if (is_numbered) {
            // 两面或者边张搭子t t+1，显然t不能是9点以上的数牌
            if (tile_get_rank(t) < 9 && cnt_table[t + 1]) {  // 两面或者边张
                work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_CHOW_OPEN_END, t);  // 记录两面或者边张搭子
                if (!is_basic_form_branch_exist(fixed_cnt, work_path, work_state)) {
                    // 削减搭子，递归
                    --cnt_table[t];
                    --cnt_table[t + 1];
                    int ret = basic_form_shanten_recursively(cnt_table, has_pair, pack_cnt, incomplete_cnt + 1,
                        fixed_cnt, work_path, work_state);
                    result = std::min(ret, result);
                    // 还原
                    ++cnt_table[t];
                    ++cnt_table[t + 1];
                }
            }
            // 嵌张搭子t t+2，显然t不能是8点以上的数牌
            if (tile_get_rank(t) < 8 && cnt_table[t + 2]) {  // 嵌张
                work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_CHOW_CLOSED, t);  // 记录嵌张搭子
                if (!is_basic_form_branch_exist(fixed_cnt, work_path, work_state)) {
                    // 削减搭子，递归
                    --cnt_table[t];
                    --cnt_table[t + 2];
                    int ret = basic_form_shanten_recursively(cnt_table, has_pair, pack_cnt, incomplete_cnt + 1,
                        fixed_cnt, work_path, work_state);
                    result = std::min(ret, result);
                    // 还原
                    ++cnt_table[t];
                    ++cnt_table[t + 2];
                }
            }
        }
    }

    if (result == max_ret) {
        save_work_path(fixed_cnt, work_path, work_state);
    }

    return result;
}

// 数牌是否有搭子
static bool numbered_tile_has_neighbor(const tile_table_t &cnt_table, tile_t t) {
    rank_t r = tile_get_rank(t);
    if (r < 9) { if (cnt_table[t + 1]) return true; }
    if (r < 8) { if (cnt_table[t + 2]) return true; }
    if (r > 1) { if (cnt_table[t - 1]) return true; }
    if (r > 2) { if (cnt_table[t - 2]) return true; }
    return false;
}

// 以表格为参数计算基本和型上听数
static int basic_form_shanten_from_table(tile_table_t &cnt_table, intptr_t fixed_cnt, useful_table_t *useful_table) {
    // 计算上听数
    work_path_t work_path;
    work_state_t work_state;
    work_state.count = 0;
    int result = basic_form_shanten_recursively(cnt_table, false, static_cast<uint16_t>(fixed_cnt), 0,
        fixed_cnt, &work_path, &work_state);

    if (useful_table == nullptr) {
        return result;
    }

    // 穷举所有的牌，获取能减少上听数的牌
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] == 4 && result > 0) {
            continue;
        }

        if (cnt_table[t] == 0) {
            // 跳过孤张字牌和不靠张的数牌，这些牌都无法减少上听数
            if (is_honor(t) || !numbered_tile_has_neighbor(cnt_table, t)) {
                continue;
            }
        }

        ++cnt_table[t];
        work_state.count = 0;
        int temp = basic_form_shanten_recursively(cnt_table, false, static_cast<uint16_t>(fixed_cnt), 0,
            fixed_cnt, &work_path, &work_state);
        if (temp < result) {
            (*useful_table)[t] = true;  // 标记为有效牌
        }
        --cnt_table[t];
    }

    return result;
}

// 基本和型上听数
int basic_form_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table) {
    if (standing_tiles == nullptr || (standing_cnt != 13
        && standing_cnt != 10 && standing_cnt != 7 && standing_cnt != 4 && standing_cnt != 1)) {
        return std::numeric_limits<int>::max();
    }

    // 对立牌的种类进行打表
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);

    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));
    }
    return basic_form_shanten_from_table(cnt_table, (13 - standing_cnt) / 3, useful_table);
}

// 基本和型判断1张是否听牌
static bool is_basic_form_wait_1(tile_table_t &cnt_table, useful_table_t *waiting_table) {
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] != 1) {
            continue;
        }

        // 单钓将
        cnt_table[t] = 0;
        if (std::all_of(std::begin(cnt_table), std::end(cnt_table), [](int n) { return n == 0; })) {
            cnt_table[t] = 1;
            if (waiting_table != nullptr) {  // 不需要获取听牌张，则可以直接返回
                (*waiting_table)[t] = true;
            }
            return true;
        }
        cnt_table[t] = 1;
    }

    return false;
}

// 基本和型判断2张是否听牌
static bool is_basic_form_wait_2(const tile_table_t &cnt_table, useful_table_t *waiting_table) {
    bool ret = false;
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 1) {
            continue;
        }
        if (cnt_table[t] > 1) {
            if (waiting_table != nullptr) {  // 获取听牌张
                (*waiting_table)[t] = true;  // 对倒
                ret = true;
                continue;
            }
            else {  // 不需要获取听牌张，则可以直接返回
                return true;
            }
        }
        if (is_numbered_suit_quick(t)) {  // 数牌搭子
            rank_t r = tile_get_rank(t);
            if (r > 1 && cnt_table[t - 1]) {  // 两面或者边张
                if (waiting_table != nullptr) {  // 获取听牌张
                    if (r < 9) (*waiting_table)[t + 1] = true;
                    if (r > 2) (*waiting_table)[t - 2] = true;
                    ret = true;
                    continue;
                }
                else {  // 不需要获取听牌张，则可以直接返回
                    return true;
                }
            }
            if (r > 2 && cnt_table[t - 2]) {  // 嵌张
                if (waiting_table != nullptr) {  // 获取听牌张
                    (*waiting_table)[t - 1] = true;
                    ret = true;
                    continue;
                }
                else {  // 不需要获取听牌张，则可以直接返回
                    return true;
                }
            }
        }
    }
    return ret;
}

// 基本和型判断4张是否听牌
static bool is_basic_form_wait_4(tile_table_t &cnt_table, useful_table_t *waiting_table) {
    bool ret = false;
    // 削减雀头
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 2) {
            continue;
        }
        // 削减雀头，递归
        cnt_table[t] -= 2;
        if (is_basic_form_wait_2(cnt_table, waiting_table)) {
            ret = true;
        }
        // 还原
        cnt_table[t] += 2;
        if (ret && waiting_table == nullptr) {  // 不需要获取听牌张，则可以直接结束递归
            return true;
        }
    }

    return ret;
}

// 递归计算基本和型是否听牌
static bool is_basic_form_wait_recursively(tile_table_t &cnt_table, intptr_t left_cnt, useful_table_t *waiting_table) {
    if (left_cnt == 1) {
        return is_basic_form_wait_1(cnt_table, waiting_table);
    }

    bool ret = false;
    if (left_cnt == 4) {
        ret = is_basic_form_wait_4(cnt_table, waiting_table);
        if (ret && waiting_table == nullptr) {  // 不需要获取听牌张，则可以直接结束递归
            return true;
        }
    }

    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 1) {
            continue;
        }

        // 刻子
        if (cnt_table[t] > 2) {
            // 削减这组刻子，递归
            cnt_table[t] -= 3;
            if (is_basic_form_wait_recursively(cnt_table, left_cnt - 3, waiting_table)) {
                ret = true;
            }
            // 还原
            cnt_table[t] += 3;
            if (ret && waiting_table == nullptr) {  // 不需要获取听牌张，则可以直接结束递归
                return true;
            }
        }

        // 顺子（只能是数牌）
        if (is_numbered_suit(t)) {
            // 顺子t t+1 t+2，显然t不能是8点以上的数牌
            if (tile_get_rank(t) < 8 && cnt_table[t + 1] && cnt_table[t + 2]) {
                // 削减这组顺子，递归
                --cnt_table[t];
                --cnt_table[t + 1];
                --cnt_table[t + 2];
                if (is_basic_form_wait_recursively(cnt_table, left_cnt - 3, waiting_table)) {
                    ret = true;
                }
                // 还原
                ++cnt_table[t];
                ++cnt_table[t + 1];
                ++cnt_table[t + 2];
                if (ret && waiting_table == nullptr) {  // 不需要获取听牌张，则可以直接结束递归
                    return true;
                }
            }
        }
    }

    return ret;
}

// 基本和型是否听牌
// 这里之所以不用直接调用上听数计算函数，判断其返回值为0的方式
// 是因为前者会削减搭子，这个操作在和牌判断中是没必要的，所以单独写一套更快逻辑
bool is_basic_form_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table) {
    // 对立牌的种类进行打表
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);

    if (waiting_table != nullptr) {
        memset(*waiting_table, 0, sizeof(*waiting_table));
    }
    return is_basic_form_wait_recursively(cnt_table, standing_cnt, waiting_table);
}

// 基本和型2张能否和牌
static bool is_basic_form_win_2(const tile_table_t &cnt_table) {
    // 找到未使用的牌
    typedef std::remove_all_extents<tile_table_t>::type table_elem_t;
    const table_elem_t *it = std::find_if(std::begin(cnt_table), std::end(cnt_table), [](table_elem_t n) { return n > 0; });
    // 存在且张数等于2
    if (it == std::end(cnt_table) || *it != 2) {
        return false;
    }
    // 还有其他未使用的牌
    return std::none_of(it + 1, std::end(cnt_table), [](int n) { return n > 0; });
}

// 递归计算基本和型是否和牌
// 这里之所以不用直接调用上听数计算函数，判断其返回值为-1的方式，
// 是因为前者会削减搭子，这个操作在和牌判断中是没必要的，所以单独写一套更快逻辑
static bool is_basic_form_win_recursively(tile_table_t &cnt_table, intptr_t left_cnt) {
    if (left_cnt == 2) {
        return is_basic_form_win_2(cnt_table);
    }

    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 1) {
            continue;
        }

        // 刻子
        if (cnt_table[t] > 2) {
            // 削减这组刻子，递归
            cnt_table[t] -= 3;
            bool ret = is_basic_form_win_recursively(cnt_table, left_cnt - 3);
            // 还原
            cnt_table[t] += 3;
            if (ret) {
                return true;
            }
        }

        // 顺子（只能是数牌）
        if (is_numbered_suit(t)) {
            // 顺子t t+1 t+2，显然t不能是8点以上的数牌
            if (tile_get_rank(t) < 8 && cnt_table[t + 1] && cnt_table[t + 2]) {
                // 削减这组顺子，递归
                --cnt_table[t];
                --cnt_table[t + 1];
                --cnt_table[t + 2];
                bool ret = is_basic_form_win_recursively(cnt_table, left_cnt - 3);
                // 还原
                ++cnt_table[t];
                ++cnt_table[t + 1];
                ++cnt_table[t + 2];
                if (ret) {
                    return true;
                }
            }
        }
    }

    return false;
}

// 基本和型是否和牌
bool is_basic_form_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile) {
    // 对立牌的种类进行打表
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);
    ++cnt_table[test_tile];  // 添加测试的牌
    return is_basic_form_win_recursively(cnt_table, standing_cnt + 1);
}

//-------------------------------- 七对 --------------------------------

// 七对上听数
int seven_pairs_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table) {
    if (standing_tiles == nullptr || standing_cnt != 13) {
        return std::numeric_limits<int>::max();
    }

    // 对牌的种类进行打表，并统计对子数
    int pair_cnt = 0;
    tile_table_t cnt_table = { 0 };
    for (intptr_t i = 0; i < standing_cnt; ++i) {
        tile_t tile = standing_tiles[i];
        ++cnt_table[tile];
        if (cnt_table[tile] == 2) {
            ++pair_cnt;
            cnt_table[tile] = 0;
        }
    }

    // 有效牌
    if (useful_table != nullptr) {
        std::transform(std::begin(cnt_table), std::end(cnt_table), std::begin(*useful_table), [](int n) { return n != 0; });
    }
    return 6 - pair_cnt;
}

// 七对是否听牌
bool is_seven_pairs_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table) {
    // 直接计算其上听数，上听数为0即为听牌
    if (waiting_table == nullptr) {
        return (0 == seven_pairs_shanten(standing_tiles, standing_cnt, nullptr));
    }

    useful_table_t useful_table;
    if (0 == seven_pairs_shanten(standing_tiles, standing_cnt, &useful_table)) {
        memcpy(*waiting_table, useful_table, sizeof(*waiting_table));
        return true;
    }
    return false;
}

// 七对是否和牌
bool is_seven_pairs_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile) {
    useful_table_t useful_table;
    return (0 == seven_pairs_shanten(standing_tiles, standing_cnt, &useful_table)
        && useful_table[test_tile]);
}

//-------------------------------- 十三幺 --------------------------------

// 十三幺上听数
int thirteen_orphans_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table) {
    if (standing_tiles == nullptr || standing_cnt != 13) {
        return std::numeric_limits<int>::max();
    }

    // 对牌的种类进行打表
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);

    bool has_pair = false;
    int cnt = 0;
    for (int i = 0; i < 13; ++i) {
        int n = cnt_table[standard_thirteen_orphans[i]];
        if (n > 0) {
            ++cnt;  // 幺九牌的种类
            if (n > 1) {
                has_pair = true;  // 幺九牌对子
            }
        }
    }

    // 当有对子时，上听数为：12-幺九牌的种类
    // 当没有对子时，上听数为：13-幺九牌的种类
    int ret = has_pair ? 12 - cnt : 13 - cnt;

    if (useful_table != nullptr) {
        // 先标记所有的幺九牌为有效牌
        memset(*useful_table, 0, sizeof(*useful_table));
        std::for_each(std::begin(standard_thirteen_orphans), std::end(standard_thirteen_orphans),
            [useful_table](tile_t t) {
            (*useful_table)[t] = true;
        });

        // 当有对子时，已有的幺九牌都不需要了
        if (has_pair) {
            for (int i = 0; i < 13; ++i) {
                tile_t t = standard_thirteen_orphans[i];
                int n = cnt_table[t];
                if (n > 0) {
                    (*useful_table)[t] = false;
                }
            }
        }
    }

    return ret;
}

// 十三幺是否听牌
bool is_thirteen_orphans_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table) {
    // 直接计算其上听数，上听数为0即为听牌
    if (waiting_table == nullptr) {
        return (0 == thirteen_orphans_shanten(standing_tiles, standing_cnt, nullptr));
    }

    useful_table_t useful_table;
    if (0 == thirteen_orphans_shanten(standing_tiles, standing_cnt, &useful_table)) {
        memcpy(*waiting_table, useful_table, sizeof(*waiting_table));
        return true;
    }
    return false;
}

// 十三幺是否和牌
bool is_thirteen_orphans_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile) {
    useful_table_t useful_table;
    return (0 == thirteen_orphans_shanten(standing_tiles, standing_cnt, &useful_table)
        && useful_table[test_tile]);
}

//-------------------------------- “组合龙+面子+雀头”和型 --------------------------------

// 以表格为参数计算组合龙是否听牌
static bool is_knitted_straight_wait_from_table(const tile_table_t &cnt_table, intptr_t left_cnt, useful_table_t *waiting_table) {
    // 匹配组合龙
    const tile_t (*matched_seq)[9] = nullptr;
    tile_t missing_tiles[9];
    int missing_cnt = 0;
    for (int i = 0; i < 6; ++i) {  // 逐个组合龙测试
        missing_cnt = 0;
        for (int k = 0; k < 9; ++k) {
            tile_t t = standard_knitted_straight[i][k];
            if (cnt_table[t] == 0) {  // 缺失的
                missing_tiles[missing_cnt++] = t;
            }
        }
        if (missing_cnt < 2) {  // 缺2张或以上的肯定没听
            matched_seq = &standard_knitted_straight[i];
            break;
        }
    }

    if (matched_seq == nullptr || missing_cnt > 2) {
        return false;
    }

    if (waiting_table != nullptr) {
        memset(*waiting_table, 0, sizeof(*waiting_table));
    }

    // 剔除组合龙
    tile_table_t temp_table;
    memcpy(&temp_table, &cnt_table, sizeof(temp_table));
    for (int i = 0; i < 9; ++i) {
        tile_t t = (*matched_seq)[i];
        if (temp_table[t]) {
            --temp_table[t];
        }
    }

    if (missing_cnt == 1) {  // 如果缺一张，那么除去组合龙之后的牌应该是完成状态才能听牌
        if (left_cnt == 10) {
            if (is_basic_form_win_recursively(temp_table, 2)) {
                if (waiting_table != nullptr) {  // 获取听牌张，听组合龙缺的一张
                    (*waiting_table)[missing_tiles[0]] = true;
                }
                return true;
            }
        }
        else {
            if (is_basic_form_win_recursively(temp_table, 5)) {
                if (waiting_table != nullptr) {  // 获取听牌张，听组合龙缺的一张
                    (*waiting_table)[missing_tiles[0]] = true;
                }
                return true;
            }
        }
    }
    else if (missing_cnt == 0) {  // 如果组合龙齐了，那么除去组合龙之后的牌要能听，整手牌才能听
        if (left_cnt == 10) {
            return is_basic_form_wait_1(temp_table, waiting_table);
        }
        else {
            return is_basic_form_wait_recursively(temp_table, 4, waiting_table);
        }
    }

    return false;
}

// 基本和型包含主番的上听数，可用于计算三步高 三同顺 龙等三组面子的番种整个立牌的上听数
static int basic_form_shanten_specified(const tile_table_t &cnt_table, const tile_t *main_tiles, int main_cnt,
    intptr_t fixed_cnt, useful_table_t *useful_table) {

    tile_table_t temp_table;
    memcpy(&temp_table, &cnt_table, sizeof(temp_table));
    int exist_cnt = 0;

    // 统计主番的牌
    for (int i = 0; i < main_cnt; ++i) {
        tile_t t = main_tiles[i];
        int n = cnt_table[t];
        if (n > 0) {  // 有，削减之
            ++exist_cnt;
            --temp_table[t];
        }
    }

    // 记录有效牌
    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));

        // 统计主番缺失的牌
        for (int i = 0; i < main_cnt; ++i) {
            tile_t t = main_tiles[i];
            int n = cnt_table[t];
            if (n <= 0) {
                (*useful_table)[t] = true;
            }
        }
    }

    // 余下牌的上听数
    int result = basic_form_shanten_from_table(temp_table, fixed_cnt + main_cnt / 3, useful_table);

    // 上听数=主番缺少的张数+余下牌的上听数
    return (main_cnt - exist_cnt) + result;
}

// 组合龙上听数
int knitted_straight_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table) {
    if (standing_tiles == nullptr || (standing_cnt != 13 && standing_cnt != 10)) {
        return std::numeric_limits<int>::max();
    }

    // 打表
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);

    int ret = std::numeric_limits<int>::max();

    // 需要获取有效牌时，计算上听数的同时就获取有效牌了
    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));

        useful_table_t temp_table;

        // 6种组合龙分别计算
        for (int i = 0; i < 6; ++i) {
            int fixed_cnt = (13 - static_cast<int>(standing_cnt)) / 3;
            int st = basic_form_shanten_specified(cnt_table, standard_knitted_straight[i], 9, fixed_cnt, &temp_table);
            if (st < ret) {  // 上听数小的，直接覆盖数据
                ret = st;
                memcpy(*useful_table, temp_table, sizeof(*useful_table));  // 直接覆盖原来的有效牌数据
            }
            else if (st == ret) {  // 两种不同组合龙上听数如果相等的话，直接合并有效牌
                std::transform(std::begin(*useful_table), std::end(*useful_table), std::begin(temp_table),
                    std::begin(*useful_table), [](bool u, bool t) { return u || t; });
            }
        }
    }
    else {
        // 6种组合龙分别计算
        for (int i = 0; i < 6; ++i) {
            int fixed_cnt = (13 - static_cast<int>(standing_cnt)) / 3;
            int st = basic_form_shanten_specified(cnt_table, standard_knitted_straight[i], 9, fixed_cnt, nullptr);
            if (st < ret) {
                ret = st;
            }
        }
    }

    return ret;
}

// 组合龙是否听牌
bool is_knitted_straight_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table) {
    if (standing_tiles == nullptr || (standing_cnt != 13 && standing_cnt != 10)) {
        return false;
    }

    // 对立牌的种类进行打表
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);

    return is_knitted_straight_wait_from_table(cnt_table, standing_cnt, waiting_table);
}

// 组合龙是否和牌
bool is_knitted_straight_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile) {
    useful_table_t waiting_table;
    return (is_knitted_straight_wait(standing_tiles, standing_cnt, &waiting_table)
        && waiting_table[test_tile]);
}

//-------------------------------- 全不靠/七星不靠 --------------------------------

// 1种组合龙的全不靠上听数
static int honors_and_knitted_tiles_shanten_1(const tile_t *standing_tiles, intptr_t standing_cnt, int which_seq, useful_table_t *useful_table) {
    if (standing_tiles == nullptr || standing_cnt != 13) {
        return std::numeric_limits<int>::max();
    }

    // 对牌的种类进行打表
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);

    int cnt = 0;

    // 统计组合龙部分的数牌
    for (int i = 0; i < 9; ++i) {
        tile_t t = standard_knitted_straight[which_seq][i];
        int n = cnt_table[t];
        if (n > 0) {  // 有，增加计数
            ++cnt;
        }
    }

    // 统计字牌
    for (int i = 6; i < 13; ++i) {
        tile_t t = standard_thirteen_orphans[i];
        int n = cnt_table[t];
        if (n > 0) {  // 有，增加计数
            ++cnt;
        }
    }

    // 记录有效牌
    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));

        // 统计组合龙部分缺失的数牌
        for (int i = 0; i < 9; ++i) {
            tile_t t = standard_knitted_straight[which_seq][i];
            int n = cnt_table[t];
            if (n <= 0) {
                (*useful_table)[t] = true;
            }
        }

        // 统计缺失的字牌
        for (int i = 6; i < 13; ++i) {
            tile_t t = standard_thirteen_orphans[i];
            int n = cnt_table[t];
            if (n <= 0) {
                (*useful_table)[t] = true;
            }
        }
    }

    // 上听数=13-符合牌型的计数
    return 13 - cnt;
}

// 全不靠上听数
int honors_and_knitted_tiles_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table) {
    int ret = std::numeric_limits<int>::max();

    // 需要获取有效牌时，计算上听数的同时就获取有效牌了
    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));

        useful_table_t temp_table;

        // 6种组合龙分别计算
        for (int i = 0; i < 6; ++i) {
            int st = honors_and_knitted_tiles_shanten_1(standing_tiles, standing_cnt, i, &temp_table);
            if (st < ret) {  // 上听数小的，直接覆盖数据
                ret = st;
                memcpy(*useful_table, temp_table, sizeof(*useful_table));  // 直接覆盖原来的有效牌数据
            }
            else if (st == ret) {  // 两种不同组合龙上听数如果相等的话，直接合并有效牌
                std::transform(std::begin(*useful_table), std::end(*useful_table), std::begin(temp_table),
                    std::begin(*useful_table), [](bool u, bool t) { return u || t; });
            }
        }
    }
    else {
        // 6种组合龙分别计算
        for (int i = 0; i < 6; ++i) {
            int st = honors_and_knitted_tiles_shanten_1(standing_tiles, standing_cnt, i, nullptr);
            if (st < ret) {
                ret = st;
            }
        }
    }
    return ret;
}

// 全不靠是否听牌
bool is_honors_and_knitted_tiles_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table) {
    // 直接计算其上听数，上听数为0即为听牌
    if (waiting_table == nullptr) {
        return (0 == honors_and_knitted_tiles_shanten(standing_tiles, standing_cnt, nullptr));
    }

    useful_table_t useful_table;
    if (0 == honors_and_knitted_tiles_shanten(standing_tiles, standing_cnt, &useful_table)) {
        memcpy(*waiting_table, useful_table, sizeof(*waiting_table));
        return true;
    }
    return false;
}

// 全不靠是否和牌
bool is_honors_and_knitted_tiles_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile) {
    useful_table_t useful_table;
    if (0 == honors_and_knitted_tiles_shanten(standing_tiles, standing_cnt, &useful_table)) {
        return useful_table[test_tile];
    }
    return false;
}

//-------------------------------- 所有情况综合 --------------------------------

bool is_waiting(const hand_tiles_t &hand_tiles, useful_table_t *useful_table) {
    bool spcial_waiting = false, basic_waiting = false;
    useful_table_t table_special, table_basic;

    if (hand_tiles.tile_count == 13) {
        if (is_thirteen_orphans_wait(hand_tiles.standing_tiles, 13, &table_special)) {
            spcial_waiting = true;
        }
        else if (is_honors_and_knitted_tiles_wait(hand_tiles.standing_tiles, 13, &table_special)) {
            spcial_waiting = true;
        }
        else if (is_seven_pairs_wait(hand_tiles.standing_tiles, 13, &table_special)) {
            spcial_waiting = true;
        }
        else if (is_knitted_straight_wait(hand_tiles.standing_tiles, 13, &table_special)) {
            spcial_waiting = true;
        }
    }
    else if (hand_tiles.tile_count == 10) {
        if (is_knitted_straight_wait(hand_tiles.standing_tiles, 10, &table_special)) {
            spcial_waiting = true;
        }
    }

    if (is_basic_form_wait(hand_tiles.standing_tiles, hand_tiles.tile_count, &table_basic)) {
        basic_waiting = true;
    }

    if (useful_table != nullptr) {
        if (spcial_waiting && basic_waiting) {
            std::transform(std::begin(table_special), std::end(table_special), std::begin(table_basic), std::begin(*useful_table),
                [](bool a, bool b) { return a || b; });
        }
        else if (basic_waiting) {
            memcpy(*useful_table, table_basic, sizeof(table_basic));
        }
        else if (spcial_waiting) {
            memcpy(*useful_table, table_special, sizeof(table_special));
        }
    }

    return (spcial_waiting || basic_waiting);
}

//-------------------------------- 枚举打牌 --------------------------------

// 枚举打哪张牌1次
static bool enum_discard_tile_1(const hand_tiles_t *hand_tiles, tile_t discard_tile, uint8_t form_flag,
    void *context, enum_callback_t enum_callback) {
    enum_result_t result;
    result.discard_tile = discard_tile;
    result.form_flag = FORM_FLAG_BASIC_FORM;
    result.shanten = basic_form_shanten(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
    if (result.shanten == 0 && result.useful_table[discard_tile]) {  // 0上听，并且打出的牌是有效牌，则修正为和了
        result.shanten = -1;
    }
    if (!enum_callback(context, &result)) {
        return false;
    }

    // 立牌有13张时，才需要计算特殊和型
    if (hand_tiles->tile_count == 13) {
        if (form_flag | FORM_FLAG_SEVEN_PAIRS) {
            result.form_flag = FORM_FLAG_SEVEN_PAIRS;
            result.shanten = seven_pairs_shanten(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
            if (result.shanten == 0 && result.useful_table[discard_tile]) {  // 0上听，并且打出的牌是有效牌，则修正为和了
                result.shanten = -1;
            }
            if (!enum_callback(context, &result)) {
                return false;
            }
        }

        if (form_flag | FORM_FLAG_THIRTEEN_ORPHANS) {
            result.form_flag = FORM_FLAG_THIRTEEN_ORPHANS;
            result.shanten = thirteen_orphans_shanten(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
            if (result.shanten == 0 && result.useful_table[discard_tile]) {  // 0上听，并且打出的牌是有效牌，则修正为和了
                result.shanten = -1;
            }
            if (!enum_callback(context, &result)) {
                return false;
            }
        }

        if (form_flag | FORM_FLAG_HONORS_AND_KNITTED_TILES) {
            result.form_flag = FORM_FLAG_HONORS_AND_KNITTED_TILES;
            result.shanten = honors_and_knitted_tiles_shanten(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
            if (result.shanten == 0 && result.useful_table[discard_tile]) {  // 0上听，并且打出的牌是有效牌，则修正为和了
                result.shanten = -1;
            }
            if (!enum_callback(context, &result)) {
                return false;
            }
        }
    }

    // 立牌有13张或者10张时，才需要计算组合龙
    if (hand_tiles->tile_count == 13 || hand_tiles->tile_count == 10) {
        if (form_flag | FORM_FLAG_KNITTED_STRAIGHT) {
            result.form_flag = FORM_FLAG_KNITTED_STRAIGHT;
            result.shanten = knitted_straight_shanten(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
            if (result.shanten == 0 && result.useful_table[discard_tile]) {  // 0上听，并且打出的牌是有效牌，则修正为和了
                result.shanten = -1;
            }
            if (!enum_callback(context, &result)) {
                return false;
            }
        }
    }

    return true;
}

// 枚举打哪张牌
void enum_discard_tile(const hand_tiles_t *hand_tiles, tile_t serving_tile, uint8_t form_flag,
    void *context, enum_callback_t enum_callback) {
    // 先计算摸切的
    if (!enum_discard_tile_1(hand_tiles, serving_tile, form_flag, context, enum_callback)) {
        return;
    }

    if (serving_tile == 0) {
        return;
    }

    // 将立牌打表
    tile_table_t cnt_table;
    map_tiles(hand_tiles->standing_tiles, hand_tiles->tile_count, &cnt_table);

    // 复制一份手牌
    hand_tiles_t temp;
    memcpy(&temp, hand_tiles, sizeof(temp));

    // 依次尝试打手中的立牌
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] && t != serving_tile && cnt_table[serving_tile] < 4) {
            --cnt_table[t];  // 打这张牌
            ++cnt_table[serving_tile];  // 上这张牌

            // 从table转成立牌
            table_to_tiles(cnt_table, temp.standing_tiles, temp.tile_count);

            // 计算
            if (!enum_discard_tile_1(&temp, t, form_flag, context, enum_callback)) {
                return;
            }

            // 复原
            --cnt_table[serving_tile];
            ++cnt_table[t];
        }
    }
}

}
