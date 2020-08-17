#ifndef __RECORD_H__
#define __RECORD_H__

#include <stddef.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>

#define NAME_SIZE 32
#define TITLE_SIZE 64

struct Record {
    char name[4][NAME_SIZE];        // 选手姓名
    struct Detail {
        uint8_t win_flag;           // 和牌选手（0123bit）
        uint8_t claim_flag;         // 点炮选手（0123bit），当点炮选手=和牌选手时，为自摸
        uint16_t fan;               // 番数
        int16_t penalty_scores[4];  // 处罚分
        bool timeout;               // 是否超时
        uint64_t fan_bits;          // 标记番种
        uint64_t fan1_bits;         // 1番13个，外加明暗杠共14个，每个番占用4bit，共56bit
        uint32_t fan2_bits;         // 2番10个，每个番占用2bit，共20bit

        struct WinHand {
            char tiles[64];         // 牌
            uint8_t win_flag;       // 和牌标记
            uint8_t flower_count;   // 花牌数
        } win_hand;         // 和牌
    } detail[16];           // 每一盘的详情
    uint8_t mode;           // 模式 0传统 1全铳
    uint16_t current_index;  // 当前打到第几盘
    time_t start_time;      // 开始时间
    time_t end_time;        // 结束时间
    char title[TITLE_SIZE]; // 对局名称
};

#define SET_WIN_CLAIM(wc_, n_) ((wc_) |= (1 << (n_)))
#define TEST_WIN_CLAIM(wc_, n_) !!((wc_) & (1 << (n_)))

#define WIN_CLAIM_INDEX(wc_) (((wc_) & 0x8) ? 3 : ((wc_) & 0x4) ? 2 : ((wc_) & 0x2) ? 1 : ((wc_) & 0x1) ? 0 : -1)

#define SET_FAN(bits_, fan_) ((bits_) |= (1ULL << (mahjong::LAST_TILE - (fan_))))
#define RESET_FAN(bits_, fan_) ((bits_) &= ~(1ULL << (mahjong::LAST_TILE - (fan_))))
#define TEST_FAN(bits_, fan_) !!((bits_) & (1ULL << (mahjong::LAST_TILE - (fan_))))

#define SET_FAN2(bits_, offset_, cnt_) ((bits_) |= ((cnt_) & 0x3U) << (static_cast<uint32_t>(offset_) << 1U))
#define RESET_FAN2(bits_, offset_) ((bits_) &= ~(0x3U << (static_cast<uint32_t>(offset_) << 1U)))
#define COUNT_FAN2(bits_, offset_) static_cast<uint8_t>(((bits_) >> (static_cast<uint32_t>(offset_) << 1U)) & 0x3U)

#define SET_FAN1(bits_, offset_, cnt_) ((bits_) |= ((cnt_) & 0xFULL) << (static_cast<uint64_t>(offset_) << 2U))
#define RESET_FAN1(bits_, offset_) ((bits_) &= ~(0xFULL << (static_cast<uint64_t>(offset_) << 2U)))
#define COUNT_FAN1(bits_, offset_) static_cast<uint8_t>(((bits_) >> (static_cast<uint64_t>(offset_) << 2U)) & 0xFULL)

void ParseRecord(const char *str, Record &record);
void StringifyRecord(std::vector<char> &str, const Record &record);

void ReadRecordFromFile(const char *file, Record &record);
void WriteRecordToFile(const char *file, const Record &record);

void SortRecords(std::vector<Record> &records);

void LoadRecordsFromString(const char *str, std::vector<Record> &records);
void SaveRecordsToString(std::vector<char> &str, const std::vector<Record> &records);

void LoadRecordsFromFile(const char *file, std::vector<Record> &records);
void SaveRecordsToFile(const char *file, const std::vector<Record> &records);
void ModifyRecordInVector(std::vector<Record> &records, const Record *r);

void TranslateDetailToScoreTable(const Record::Detail &detail, uint8_t mode, int (&scoreTable)[4]);
void CalculateRankFromScore(const int (&scores)[4], unsigned (&ranks)[4]);
void RankToStandardScore(const unsigned (&ranks)[4], unsigned (&ss12)[4]);

#endif
