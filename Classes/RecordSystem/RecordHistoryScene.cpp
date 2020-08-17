#include "RecordHistoryScene.h"
#include <array>
#include <regex>
#include "Record.h"
#include "../UICommon.h"
#include "../UIColors.h"
#include "../widget/AlertDialog.h"
#include "../widget/Toast.h"
#include "../widget/LoadingView.h"
#include "../widget/PopupMenu.h"
#include "../widget/DatePicker.h"
#include "../utils/socket.h"

USING_NS_CC;

#define NO_NAME_TITLE __UTF8("(未命名对局)")

#define C4B_RED cocos2d::Color4B(254, 87, 110, 255)

#define SECONDS_PER_DAY 86400

static bool g_hasLoaded = false;
static std::vector<Record> g_records;

static void loadRecords(std::vector<Record> &records) {
    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("history_record.json");
    LoadRecordsFromFile(fileName.c_str(), records);
}

static void saveRecords(const std::vector<Record> &records) {
    std::string fileName = FileUtils::getInstance()->getWritablePath();
    fileName.append("history_record.json");
    SaveRecordsToFile(fileName.c_str(), records);
}

#define BUF_SIZE 511

static std::string formatTime(time_t startTime, time_t endTime) {
    char str[BUF_SIZE + 1];
    str[BUF_SIZE] = '\0';

    struct tm ret = *localtime(&startTime);
    int len = snprintf(str, BUF_SIZE, __UTF8("%d年%d月%d日%.2d:%.2d"), ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min);
    if (endTime != 0) {
        ret = *localtime(&endTime);
        len += snprintf(str + len, static_cast<size_t>(BUF_SIZE - len), __UTF8("——%d年%d月%d日%.2d:%.2d"), ret.tm_year + 1900, ret.tm_mon + 1, ret.tm_mday, ret.tm_hour, ret.tm_min);
    }
    else {
        len += snprintf(str + len, static_cast<size_t>(BUF_SIZE - len), __UTF8("——(未结束)"));
    }
    return str;
}

static std::string formatTitle(const Record &record) {
    return std::string(record.mode == 0 ? __UTF8("[翻三] ") : __UTF8("[全铳] ")) + (record.title[0] != '\0' ? record.title : NO_NAME_TITLE);
}

void RecordHistoryScene::updateRecordTexts() {
    _recordTexts.clear();
    _recordTexts.reserve(g_records.size());

    std::transform(g_records.begin(), g_records.end(), std::back_inserter(_recordTexts), [](const Record &record) {
        RecordTexts texts;
        texts.source = &record;
        texts.title = formatTitle(record);
        texts.time = formatTime(record.start_time, record.end_time);

        typedef std::pair<int, int> SeatScore;
        SeatScore seatscore[4];
        seatscore[0].first = 0; seatscore[0].second = 0;
        seatscore[1].first = 1, seatscore[1].second = 0;
        seatscore[2].first = 2, seatscore[2].second = 0;
        seatscore[3].first = 3, seatscore[3].second = 0;
        for (int i = 0; i < 16; ++i) {
            int s[4];
            TranslateDetailToScoreTable(record.detail[i], record.mode, s);
            seatscore[0].second += s[0];
            seatscore[1].second += s[1];
            seatscore[2].second += s[2];
            seatscore[3].second += s[3];
        }

        std::stable_sort(std::begin(seatscore), std::end(seatscore),
            [](const SeatScore &left, const SeatScore &right) {
            return left.second > right.second;
        });

        static const char *seatText[] = { __UTF8("东"), __UTF8("南"), __UTF8("西"), __UTF8("北") };
        for (int i = 0; i < 4; ++i) {
            int seat = seatscore[i].first;
            int score = seatscore[i].second;
            texts.players[i] = Common::format("%s: %s (%+d)", seatText[seat], record.name[seat], score);
            texts.seats[i] = seat;
        }

        return texts;
    });
}

bool RecordHistoryScene::init(ViewCallback &&viewCallback) {
    if (UNLIKELY(!BaseScene::initWithTitle(__UTF8("历史记录")))) {
        return false;
    }

    memset(&_filterCriteria, 0, sizeof(_filterCriteria));
    _viewCallback.swap(viewCallback);

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 更多按钮
    ui::Button *button = cocos2d::ui::Button::create("icon/menu.png");
    this->addChild(button);
    button->setScale(20.0f / button->getContentSize().width);
    button->setPosition(Vec2(origin.x + visibleSize.width - 15.0f, origin.y + visibleSize.height - 15.0f));
    button->addClickEventListener(std::bind(&RecordHistoryScene::onMoreButton, this, std::placeholders::_1));
    _moreButton = button;

    Label *label = Label::createWithSystemFont(__UTF8("无历史记录"), "Arial", 12);
    this->addChild(label);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 15.0f));
    label->setTextColor(C4B_BLACK);
    label->setVisible(false);
    _emptyLabel = label;

    cw::TableView *tableView = cw::TableView::create();
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setScrollBarPositionFromCorner(Vec2(2.0f, 2.0f));
    tableView->setScrollBarWidth(4.0f);
    tableView->setScrollBarOpacity(0x99);
    tableView->setContentSize(Size(visibleSize.width - 5.0f, visibleSize.height - 35.0f));
    tableView->setDelegate(this);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 15.0f));
    this->addChild(tableView);
    _tableView = tableView;

    if (UNLIKELY(!g_hasLoaded)) {
        this->scheduleOnce([this](float) {
            LoadingView *loadingView = LoadingView::create();
            loadingView->showInScene(this);

            auto thiz = makeRef(this);  // 保证线程回来之前不析构

            auto records = std::make_shared<std::vector<Record> >();
            AsyncTaskPool::getInstance()->enqueue(AsyncTaskPool::TaskType::TASK_IO, [records, thiz, loadingView](void *) {
                g_records.swap(*records);
                g_hasLoaded = true;

                if (LIKELY(thiz->isRunning())) {
                    thiz->updateRecordTexts();
                    thiz->refresh();
                    loadingView->dismiss();
                }
            }, nullptr, [records]() { loadRecords(*records); });
        }, 0.0f, "load_records");
    }

    return true;
}

#ifdef _MSC_VER

const char *strcasestr(const char *haystack, const char *needle) {
    size_t len = strlen(needle);
    while (*haystack != '\0') {
        if (_strnicmp(haystack, needle, len) == 0) {
            return haystack;
        }
        ++haystack;
    }
    return nullptr;
}

#endif

void RecordHistoryScene::filter() {
    if (g_records.empty()) {
        _filterIndices.clear();
        return;
    }

    // first指向数据源，second存放匹配人名的位标记
    std::vector<std::pair<const Record *, uint8_t> > temp1;
    temp1.reserve(g_records.size());
    std::transform(g_records.begin(), g_records.end(), std::back_inserter(temp1), [](const Record &r) { return std::make_pair(&r, 0); });

    std::vector<std::pair<const Record *, uint8_t> > temp2;
    temp2.reserve(g_records.size());

    // 筛选起始时间
    time_t startTime = _filterCriteria.start_time;
    if (!temp1.empty() && startTime != 0) {
        std::copy_if(temp1.begin(), temp1.end(), std::back_inserter(temp2), [startTime](const std::pair<const Record *, uint8_t> &d) { return d.first->start_time >= startTime; });
        temp1.swap(temp2);
        temp2.clear();
    }

    // 筛选截止时间
    time_t finishTime = _filterCriteria.finish_time;
    if (!temp1.empty() && finishTime != 0) {
        finishTime += SECONDS_PER_DAY;
        std::copy_if(temp1.begin(), temp1.end(), std::back_inserter(temp2), [finishTime](const std::pair<const Record *, uint8_t> &d) { return d.first->end_time < finishTime; });
        temp1.swap(temp2);
        temp2.clear();
    }

    if (_filterCriteria.regular_enabled) {  // 正则匹配
        // 筛选选手姓名
        if (!temp1.empty() && !Common::isCStringEmpty(_filterCriteria.name)) {
            std::regex_constants::syntax_option_type type = std::regex_constants::ECMAScript;
            if (_filterCriteria.ignore_case) type |= std::regex_constants::icase;  // 忽略大小写

            std::regex reg(_filterCriteria.name, type);
            if (_filterCriteria.whole_word) {  // 全词匹配
                std::copy_if(temp1.begin(), temp1.end(), std::back_inserter(temp2), [&reg](std::pair<const Record *, uint8_t> &d) {
                    uint8_t flag = 0;
                    if (std::regex_match(d.first->name[0], reg)) flag |= 0x1;
                    if (std::regex_match(d.first->name[1], reg)) flag |= 0x2;
                    if (std::regex_match(d.first->name[2], reg)) flag |= 0x4;
                    if (std::regex_match(d.first->name[3], reg)) flag |= 0x8;
                    d.second = flag;
                    return (flag != 0);
                });
            }
            else {  // 部分匹配
                std::copy_if(temp1.begin(), temp1.end(), std::back_inserter(temp2), [&reg](std::pair<const Record *, uint8_t> &d) {
                    uint8_t flag = 0;
                    if (std::regex_search(d.first->name[0], reg)) flag |= 0x1;
                    if (std::regex_search(d.first->name[1], reg)) flag |= 0x2;
                    if (std::regex_search(d.first->name[2], reg)) flag |= 0x4;
                    if (std::regex_search(d.first->name[3], reg)) flag |= 0x8;
                    d.second = flag;
                    return (flag != 0);
                });
            }
            temp1.swap(temp2);
            temp2.clear();
        }

        // 筛选对局名称
        if (!temp1.empty() && !Common::isCStringEmpty(_filterCriteria.title)) {
            std::regex_constants::syntax_option_type type = std::regex_constants::ECMAScript;
            if (_filterCriteria.ignore_case) type |= std::regex_constants::icase;  // 忽略大小写

            std::regex reg(_filterCriteria.title, type);
            if (_filterCriteria.whole_word) {  // 全词匹配
                std::copy_if(temp1.begin(), temp1.end(), std::back_inserter(temp2), [&reg](std::pair<const Record *, uint8_t> &d) {
                    return std::regex_match(d.first->title, reg);
                });
            }
            else {  // 部分匹配
                std::copy_if(temp1.begin(), temp1.end(), std::back_inserter(temp2), [&reg](std::pair<const Record *, uint8_t> &d) {
                    return std::regex_search(d.first->title, reg);
                });
            }
            temp1.swap(temp2);
            temp2.clear();
        }
    }
    else {  // 普通匹配
        // 筛选选手姓名
        if (!temp1.empty() && !Common::isCStringEmpty(_filterCriteria.name)) {
            const char *str = _filterCriteria.name;
            if (_filterCriteria.ignore_case) {  // 忽略大小写
                if (_filterCriteria.whole_word) {  // 全词匹配
                    std::copy_if(temp1.begin(), temp1.end(), std::back_inserter(temp2), [str](std::pair<const Record *, uint8_t> &d) {
                        uint8_t flag = 0;
                        if (strcasecmp(d.first->name[0], str) == 0) flag |= 0x1;
                        if (strcasecmp(d.first->name[1], str) == 0) flag |= 0x2;
                        if (strcasecmp(d.first->name[2], str) == 0) flag |= 0x4;
                        if (strcasecmp(d.first->name[3], str) == 0) flag |= 0x8;
                        d.second = flag;
                        return (flag != 0);
                    });
                }
                else {  // 部分匹配
                    std::copy_if(temp1.begin(), temp1.end(), std::back_inserter(temp2), [str](std::pair<const Record *, uint8_t> &d) {
                        uint8_t flag = 0;
                        if (strcasestr(d.first->name[0], str) != nullptr) flag |= 0x1;
                        if (strcasestr(d.first->name[1], str) != nullptr) flag |= 0x2;
                        if (strcasestr(d.first->name[2], str) != nullptr) flag |= 0x4;
                        if (strcasestr(d.first->name[3], str) != nullptr) flag |= 0x8;
                        d.second = flag;
                        return (flag != 0);
                    });
                }
            }
            else {
                if (_filterCriteria.whole_word) {  // 全词匹配
                    std::copy_if(temp1.begin(), temp1.end(), std::back_inserter(temp2), [str](std::pair<const Record *, uint8_t> &d) {
                        uint8_t flag = 0;
                        if (strcmp(d.first->name[0], str) == 0) flag |= 0x1;
                        if (strcmp(d.first->name[1], str) == 0) flag |= 0x2;
                        if (strcmp(d.first->name[2], str) == 0) flag |= 0x4;
                        if (strcmp(d.first->name[3], str) == 0) flag |= 0x8;
                        d.second = flag;
                        return (flag != 0);
                    });
                }
                else {  // 部分匹配
                    std::copy_if(temp1.begin(), temp1.end(), std::back_inserter(temp2), [str](std::pair<const Record *, uint8_t> &d) {
                        uint8_t flag = 0;
                        if (strstr(d.first->name[0], str) != nullptr) flag |= 0x1;
                        if (strstr(d.first->name[1], str) != nullptr) flag |= 0x2;
                        if (strstr(d.first->name[2], str) != nullptr) flag |= 0x4;
                        if (strstr(d.first->name[3], str) != nullptr) flag |= 0x8;
                        d.second = flag;
                        return (flag != 0);
                    });
                }
            }
            temp1.swap(temp2);
            temp2.clear();
        }

        // 筛选对局名称
        if (!temp1.empty() && !Common::isCStringEmpty(_filterCriteria.title)) {
            const char *str = _filterCriteria.title;
            if (_filterCriteria.ignore_case) {  // 忽略大小写
                if (_filterCriteria.whole_word) {  // 全词匹配
                    std::copy_if(temp1.begin(), temp1.end(), std::back_inserter(temp2), [str](std::pair<const Record *, uint8_t> &d) {
                        return (strcasecmp(d.first->title, str) == 0);
                    });
                }
                else {  // 部分匹配
                    std::copy_if(temp1.begin(), temp1.end(), std::back_inserter(temp2), [str](std::pair<const Record *, uint8_t> &d) {
                        return (strcasestr(d.first->title, str) != nullptr);
                    });
                }
            }
            else {
                if (_filterCriteria.whole_word) {  // 全词匹配
                    std::copy_if(temp1.begin(), temp1.end(), std::back_inserter(temp2), [str](std::pair<const Record *, uint8_t> &d) {
                        return (strcmp(d.first->title, str) == 0);
                    });
                }
                else {  // 部分匹配
                    std::copy_if(temp1.begin(), temp1.end(), std::back_inserter(temp2), [str](std::pair<const Record *, uint8_t> &d) {
                        return (strstr(d.first->title, str) != nullptr);
                    });
                }
            }
            temp1.swap(temp2);
            temp2.clear();
        }
    }

    _filterIndices.clear();
    const Record *p = &g_records[0];
    std::transform(temp1.begin(), temp1.end(), std::back_inserter(_filterIndices), [p](const std::pair<const Record *, uint8_t> &d) {
        return FilterIndex({ static_cast<size_t>(d.first - p), d.second });  // first转换为下标
    });
}

void RecordHistoryScene::refresh() {
    try {
        filter();
    }
    catch (std::regex_error &) {
        Toast::makeText(this, __UTF8("正则表达式错误，请尝试重置筛选条件"), Toast::Duration::LENGTH_LONG)->show();
        return;
    }

    _tableView->reloadDataInplacement();
    _emptyLabel->setVisible(_filterIndices.empty());
}

void RecordHistoryScene::onEnter() {
    BaseScene::onEnter();

    if (LIKELY(g_hasLoaded)) {
        updateRecordTexts();
        refresh();
    }
}

ssize_t RecordHistoryScene::numberOfCellsInTableView(cw::TableView *) {
    return _filterIndices.size();
}

float RecordHistoryScene::tableCellSizeForIndex(cw::TableView *, ssize_t) {
    return 65.0f;
}

cw::TableViewCell *RecordHistoryScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    typedef cw::TableViewCellEx<std::array<LayerColor *, 2>, Label *, Label *, std::array<Label *, 4>, ui::Button *> CustomCell;
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    const float cellWidth = table->getContentSize().width;

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        LayerColor **layerColors = std::get<0>(ext).data();
        Label *&titleLabel = std::get<1>(ext);
        Label *&timeLabel = std::get<2>(ext);
        Label **playerLabels = std::get<3>(ext).data();
        ui::Button *&delBtn = std::get<4>(ext);

        // 背景色
        layerColors[0] = LayerColor::create(Color4B(0x10, 0x10, 0x10, 0x10), cellWidth, 65.0f);
        cell->addChild(layerColors[0]);

        layerColors[1] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), cellWidth, 65.0f);
        cell->addChild(layerColors[1]);

        // 标题
        Label *label = Label::createWithSystemFont("", "Arail", 10);
        label->setTextColor(C4B_BLACK);
        cell->addChild(label);
        label->setPosition(Vec2(2.0f, 55.0f));
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        titleLabel = label;

        // 时间
        label = Label::createWithSystemFont("", "Arail", 10);
        label->setTextColor(C4B_GRAY);
        cell->addChild(label);
        label->setPosition(Vec2(2.0f, 40.0f));
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        timeLabel = label;

        // 四名选手
        for (int i = 0; i < 4; ++i) {
            label = Label::createWithSystemFont("", "Arail", 10);
            cell->addChild(label);
            label->setPosition(Vec2(2.0f + (i & 1) * cellWidth * 0.5f, 23.0f - (i >> 1) * 15.0f));
            label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
            playerLabels[i] = label;
        }

        ui::Button *button = ui::Button::create("icon/delete.png");
        button->setScale(20.0f / button->getContentSize().width);
        button->setColor(C3B_GRAY);
        button->addClickEventListener(std::bind(&RecordHistoryScene::onDeleteButton, this, std::placeholders::_1));
        cell->addChild(button);
        button->setPosition(Vec2(cellWidth - 15.0f, 40.0f));
        delBtn = button;

        cell->setContentSize(Size(cellWidth, 65.0f));
        cell->setTouchEnabled(true);
        cell->addClickEventListener(std::bind(&RecordHistoryScene::onCellClicked, this, std::placeholders::_1));
    }

    const CustomCell::ExtDataType &ext = cell->getExtData();
    LayerColor *const *layerColors = std::get<0>(ext).data();
    Label *titleLabel = std::get<1>(ext);
    Label *timeLabel = std::get<2>(ext);
    Label *const *playerLabels = std::get<3>(ext).data();
    ui::Button *delBtn = std::get<4>(ext);

    layerColors[0]->setVisible((idx & 1) == 0);
    layerColors[1]->setVisible((idx & 1) != 0);

    const FilterIndex &data = _filterIndices[idx];
    size_t realIdx = data.real_index;
    delBtn->setUserData(reinterpret_cast<void *>(realIdx));
    cell->setUserData(reinterpret_cast<void *>(realIdx));

    const RecordTexts &texts = _recordTexts[realIdx];
    titleLabel->setString(texts.title);
    cw::scaleLabelToFitWidth(titleLabel, cellWidth - 4.0f);

    timeLabel->setString(texts.time);
    cw::scaleLabelToFitWidth(timeLabel, cellWidth - 30.0f);

    uint8_t flag = data.player_flag;
    for (int i = 0; i < 4; ++i) {
        Label *label = playerLabels[i];
        label->setString(texts.players[i]);
        label->setTextColor(((1U << texts.seats[i]) & flag) ? C4B_RED : C4B_GRAY);
        cw::scaleLabelToFitWidth(label, cellWidth * 0.5f - 4.0f);
    }

    return cell;
}

void RecordHistoryScene::onDeleteButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    size_t idx = reinterpret_cast<size_t>(button->getUserData());

    std::string msg = __UTF8("你将删除记录：\n\n");
    msg += _recordTexts[idx].title;
    msg += '\n';
    msg += _recordTexts[idx].time;
    for (int i = 0; i < 4; ++i) {
        msg += '\n';
        msg += _recordTexts[idx].players[i];
    }
    msg += __UTF8("\n\n删除后无法找回，确认删除？");

    AlertDialog::Builder(this)
        .setTitle(__UTF8("警告"))
        .setMessage(std::move(msg))
        .setNegativeButton(__UTF8("取消"), nullptr)
        .setPositiveButton(__UTF8("确定"), [this, idx](AlertDialog *, int) {
        g_records.erase(g_records.begin() + idx);
        saveRecordsAndRefresh();
        return true;
    }).create()->show();
}

void RecordHistoryScene::saveRecordsAndRefresh() {
    LoadingView *loadingView = LoadingView::create();
    loadingView->showInScene(this);

    auto thiz = makeRef(this);  // 保证线程回来之前不析构

    auto records = std::make_shared<std::vector<Record> >(g_records);
    AsyncTaskPool::getInstance()->enqueue(AsyncTaskPool::TaskType::TASK_IO, [thiz, loadingView](void *) {
        if (LIKELY(thiz->isRunning())) {
            thiz->updateRecordTexts();
            thiz->refresh();
            loadingView->dismiss();
        }
    }, nullptr, [records]() { saveRecords(*records); });
}

void RecordHistoryScene::onMoreButton(cocos2d::Ref *sender) {
    if (UNLIKELY(!g_hasLoaded)) {
        Toast::makeText(this, __UTF8("请等待加载完成"), Toast::Duration::LENGTH_LONG)->show();
        return;
    }

    Vec2 pos = ((ui::Button *)sender)->getPosition();
    pos.y -= 15.0f;
    PopupMenu *menu = PopupMenu::create(this, { __UTF8("筛选条件"), __UTF8("个人汇总"), __UTF8("批量删除"), __UTF8("点对点传输") }, pos, Vec2::ANCHOR_TOP_RIGHT);
    menu->setMenuItemCallback([this](PopupMenu *, size_t idx) {
        if (UNLIKELY(g_records.empty() && idx != 3)) {
            Toast::makeText(this, __UTF8("无历史记录"), Toast::Duration::LENGTH_LONG)->show();
            return;
        }

        switch (idx) {
        case 0: showFilterAlert(); break;
        case 1: switchToSummary(); break;
        case 2: switchToBatchDelete(); break;
        case 3: showTransmissionAlert(); break;
        default: UNREACHABLE(); break;
        }
    });
    menu->show();
}

static const char *reportRegexError(const std::regex_error &e) {
    switch (e.code()) {
    case std::regex_constants::error_collate: return __UTF8("表达式中包含无效的排序规则元素名");
    case std::regex_constants::error_ctype: return __UTF8("表达式中包含无效的字符类名");
    case std::regex_constants::error_escape: return __UTF8("表达式包含无效转义序列");
    case std::regex_constants::error_backref: return __UTF8("表达式中包含无效的回溯引用");
    case std::regex_constants::error_brack: return __UTF8("表达式中包含不匹配的“[”或“]”");
    case std::regex_constants::error_paren: return __UTF8("表达式中包含不匹配的“(”或“)”");
    case std::regex_constants::error_brace: return __UTF8("表达式中包含不匹配的“{”或“}”");
    case std::regex_constants::error_badbrace: return __UTF8("表达式在 { } 表达式中包含无效计数");
    case std::regex_constants::error_range: return __UTF8("表达式中包含无效的字符范围说明符");
    case std::regex_constants::error_space: return __UTF8("分析正则表达式失败，因为没有足够的资源");
    case std::regex_constants::error_badrepeat: return __UTF8("重复表达式签名没有表达式");
    case std::regex_constants::error_complexity: return __UTF8("尝试的匹配的复杂度超过了预定义的等级");
    case std::regex_constants::error_stack: return __UTF8("尝试匹配失败，因为没有足够的内存");
    default: return __UTF8("未知错误");
    }
}

void RecordHistoryScene::showFilterAlert() {
    const float limitWidth = AlertDialog::maxWidth();

    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(limitWidth, 170.0f));

    std::array<ui::CheckBox *, 4> checkBoxes;
    std::array<ui::Button *, 2> buttons;
    std::array<ui::EditBox *, 2> editBoxes;

    bool dateEnabled = (_filterCriteria.start_time != 0 || _filterCriteria.finish_time != 0);

    ui::CheckBox *checkBox = UICommon::createCheckBox();
    checkBox->setZoomScale(0.0f);
    checkBox->ignoreContentAdaptWithSize(false);
    checkBox->setContentSize(Size(20.0f, 20.0f));
    rootNode->addChild(checkBox);
    checkBox->setPosition(Vec2(10.0f, 160.0f));
    checkBox->setSelected(dateEnabled);
    checkBoxes[0] = checkBox;

    // 文本+日期控件
    static const char *titleText1[] = { __UTF8("起始日期"), __UTF8("截止日期") };
    const float buttonWidth = (limitWidth - 30.0f) * 0.5f;
    for (int i = 0; i < 2; ++i) {
        Label *label = Label::createWithSystemFont(titleText1[i], "Arial", 10);
        label->setTextColor(C4B_GRAY);
        rootNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(25.0f + (5.0f + buttonWidth) * i, 160.0f));

        ui::Button *button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
        button->setScale9Enabled(true);
        button->setContentSize(Size(buttonWidth, 30.0f));
        button->setTitleColor(C3B_GRAY);
        button->setTitleFontSize(14);
        button->setEnabled(dateEnabled);
        rootNode->addChild(button);
        button->setPosition(Vec2(25.0f + buttonWidth * 0.5f + (5.0f + buttonWidth) * i, 135.0f));
        button->setTag(i);
        buttons[i] = button;
    }

    auto setupButton = [](ui::Button *button, time_t t) {
        struct tm tms = *localtime(&t);
        char str[64];
        snprintf(str, sizeof(str), "%d-%.2d-%.2d", tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday);
        button->setTitleText(str);
        if (tms.tm_hour != 0 || tms.tm_min != 0 || tms.tm_sec != 0) {  // 规整到一天的00：00：00
            tms.tm_hour = 0;
            tms.tm_min = 0;
            tms.tm_sec = 0;
            t = mktime(&tms);
        }
        button->setUserData(reinterpret_cast<void *>(t));
    };
    setupButton(buttons[0], _filterCriteria.start_time != 0 ? _filterCriteria.start_time : time(nullptr) - 7 * SECONDS_PER_DAY);
    setupButton(buttons[1], _filterCriteria.finish_time != 0 ? _filterCriteria.finish_time : time(nullptr));

    std::function<void (Ref *)> onButton = [this](Ref *sender) {
        ui::Button *button = (ui::Button *)sender;
        time_t t = reinterpret_cast<time_t>(button->getUserData());
        struct tm ts = *localtime(&t);
        DatePicker::Date date;
        date.year = ts.tm_year + 1900;
        date.month = ts.tm_mon + 1;
        date.day = ts.tm_mday;
        DatePicker::create(&date, [button](DatePicker *dp, bool confirm) {
            if (!confirm) return;

            const DatePicker::Date &date = dp->getDate();
            char str[64];
            snprintf(str, sizeof(str), "%d-%.2d-%.2d", date.year, date.month, date.day);
            button->setTitleText(str);

            struct tm ts = { 0 };
            ts.tm_year = date.year - 1900;
            ts.tm_mon = date.month - 1;
            ts.tm_mday = date.day;
            time_t t = mktime(&ts);
            button->setUserData(reinterpret_cast<void *>(t));
        })->showInScene(this);
    };
    buttons[0]->addClickEventListener(onButton);
    buttons[1]->addClickEventListener(onButton);

    checkBoxes[0]->addEventListener([buttons](Ref *, ui::CheckBox::EventType event) {
        if (event == ui::CheckBox::EventType::SELECTED) {
            buttons[0]->setEnabled(true);
            buttons[1]->setEnabled(true);
        }
        else {
            buttons[0]->setEnabled(false);
            buttons[1]->setEnabled(false);
        }
    });

    // 文本+输入框
    static const char *titleText2[] = { __UTF8("选手姓名"), __UTF8("对局名称") };
    for (int i = 0; i < 2; ++i) {
        const float yPos = 100.0f - i * 25.0f;
        Label *label = Label::createWithSystemFont(titleText2[i], "Arial", 12);
        label->setTextColor(C4B_BLACK);
        rootNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(0.0f, yPos));

        ui::EditBox *editBox = UICommon::createEditBox(Size(limitWidth - 55.0f, 20.0f));
        editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
        editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
        editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
        editBox->setFontColor(C4B_BLACK);
        editBox->setFontSize(12);
        rootNode->addChild(editBox);
        editBox->setPosition(Vec2(limitWidth * 0.5f + 27.5f, yPos));
        editBoxes[i] = editBox;
    }

    editBoxes[0]->setMaxLength(sizeof(_filterCriteria.name) - 1);
    editBoxes[0]->setText(_filterCriteria.name);
    editBoxes[1]->setMaxLength(sizeof(_filterCriteria.title) - 1);
    editBoxes[1]->setText(_filterCriteria.title);

    // 忽略大小写 & 全词匹配
    static const char *titleText3[] = { __UTF8("忽略大小写"), __UTF8("全词匹配") };
    for (int i = 0; i < 2; ++i) {
        checkBox = UICommon::createCheckBox();
        checkBox->setZoomScale(0.0f);
        checkBox->ignoreContentAdaptWithSize(false);
        checkBox->setContentSize(Size(20.0f, 20.0f));
        rootNode->addChild(checkBox);
        checkBox->setPosition(Vec2(10.0f + i * limitWidth * 0.5f, 45.0f));
        checkBoxes[1 + i] = checkBox;

        Label *label = Label::createWithSystemFont(titleText3[i], "Arial", 12);
        label->setTextColor(C4B_GRAY);
        rootNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(25.0f + i * limitWidth * 0.5f, 45.0f));
    }
    checkBoxes[1]->setSelected(_filterCriteria.ignore_case);
    checkBoxes[2]->setSelected(_filterCriteria.whole_word);

    checkBox = UICommon::createCheckBox();
    checkBox->setZoomScale(0.0f);
    checkBox->ignoreContentAdaptWithSize(false);
    checkBox->setContentSize(Size(15.0f, 15.0f));
    rootNode->addChild(checkBox);
    checkBox->setPosition(Vec2(7.5f, 10.0f));
    checkBox->setSelected(_filterCriteria.regular_enabled);
    checkBoxes[3] = checkBox;

    Label *label = Label::createWithSystemFont(__UTF8("正则（如果你不知道正则是什么，请勿勾选）"), "Arial", 10);
    label->setTextColor(C4B_GRAY);
    rootNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(20.0f, 10.0f));
    cw::scaleLabelToFitWidth(label, limitWidth - 20.0f);

    AlertDialog::Builder(this)
        .setTitle(__UTF8("筛选条件"))
        .setContentNode(rootNode)
        .setNegativeButton(__UTF8("取消"), nullptr)
        .setPositiveButton(__UTF8("确定"), [this, buttons, editBoxes, checkBoxes](AlertDialog *, int) {
        FilterCriteria temp = { 0 };
        if (checkBoxes[0]->isSelected()) {
            temp.start_time = reinterpret_cast<time_t>(buttons[0]->getUserData());
            temp.finish_time = reinterpret_cast<time_t>(buttons[1]->getUserData());
        }
        strncpy(temp.name, editBoxes[0]->getText(), sizeof(temp.name) - 1);
        strncpy(temp.title, editBoxes[1]->getText(), sizeof(temp.title) - 1);
        temp.ignore_case = checkBoxes[1]->isSelected();
        temp.whole_word = checkBoxes[2]->isSelected();
        temp.regular_enabled = checkBoxes[3]->isSelected();

        if (memcmp(&temp, &_filterCriteria, sizeof(temp)) != 0) {  // 筛选条件发生改变
            std::swap(temp, _filterCriteria);
            try {
                filter();
            }
            catch (std::regex_error &e) {
                // 正则表达式抛异常，恢复原来的筛选条件
                std::swap(temp, _filterCriteria);
                const char *error = reportRegexError(e);
                Toast::makeText(this, error, Toast::Duration::LENGTH_LONG)->show();
                return false;
            }
            _tableView->reloadDataInplacement();
            _emptyLabel->setVisible(_filterIndices.empty());
        }
        return true;
    }).create()->show();
}

namespace {
    struct RecordsStatistic {
        unsigned competition_count;
        unsigned hand_count;
        unsigned rank[4];
        unsigned standard_score12;
        int competition_score;
        uint16_t max_fan;
        unsigned win;
        unsigned self_drawn;
        unsigned claim;
        unsigned win_fan;
        unsigned claim_fan;
    };
}

static void SummarizeRecords(const std::vector<int8_t> &flags, const std::vector<Record> &records, RecordsStatistic *result) {
    memset(result, 0, sizeof(*result));

    for (size_t i = 0, cnt = std::min<size_t>(flags.size(), records.size()); i < cnt; ++i) {
        const Record &record = records[i];
        if (record.end_time == 0) {
            continue;
        }

        int8_t idx = flags[i];
        if (idx == -1) {
            continue;
        }

        ++result->competition_count;
        int totalScores[4] = { 0 };

        for (int k = 0; k < 16; ++k) {
            const Record::Detail &detail = record.detail[k];
            if (UNLIKELY(detail.timeout)) {
                continue;
            }
            ++result->hand_count;

            uint16_t fan = detail.fan;
            uint8_t wf = detail.win_flag;
            uint8_t cf = detail.claim_flag;
            if (fan == 0 || wf == 0 || cf == 0) {
                continue;
            }

            int scoreTable[4];
            TranslateDetailToScoreTable(detail, record.mode, scoreTable);
            for (int n = 0; n < 4; ++n) {
                totalScores[n] += scoreTable[n];
            }

            int winIndex = WIN_CLAIM_INDEX(wf);
            int claimIndex = WIN_CLAIM_INDEX(cf);

            if (winIndex == idx) {
                ++result->win;
                if (claimIndex == idx) {
                    ++result->self_drawn;
                }
                result->win_fan += fan;
                result->max_fan = std::max(result->max_fan, fan);
            }
            else if (claimIndex == idx) {
                ++result->claim;
                result->claim_fan += fan;
            }
        }

        unsigned ranks[4];
        CalculateRankFromScore(totalScores, ranks);
        ++result->rank[ranks[idx]];
        result->competition_score += totalScores[idx];

        unsigned ss[4];
        RankToStandardScore(ranks, ss);
        result->standard_score12 += ss[idx];
    }
}

static cocos2d::Node *createStatisticNode(const RecordsStatistic &rs) {
    const float width = AlertDialog::maxWidth();
    const float height = 135.0f;

    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(width, height));

    std::string texts[16];
    for (int i = 0; i < 4; ++i) {
        texts[i] = Common::format(__UTF8("%d位：%2u (%.2f%%)"), i + 1, rs.rank[i], rs.competition_count > 0 ? rs.rank[i] * 100 / static_cast<float>(rs.competition_count) : 0.0f);
    }
    texts[4] = Common::format(__UTF8("标准分：%.2f"), rs.standard_score12 / 12.0f);
    texts[5] = Common::format(__UTF8("比赛分：%d"), rs.competition_score);
    texts[6] = Common::format(__UTF8("平均标准分：%.2f"), rs.competition_count > 0 ? rs.standard_score12 / static_cast<float>(rs.competition_count * 12) : 0.0f);
    texts[7] = Common::format(__UTF8("平均比赛分：%.2f"), rs.competition_count > 0 ? rs.competition_score / static_cast<float>(rs.competition_count) : 0.0f);
    texts[8] = Common::format(__UTF8("和牌率：%.2f%%"), rs.hand_count > 0 ? rs.win * 100 / static_cast<float>(rs.hand_count) : 0.0f);
    texts[9] = Common::format(__UTF8("点炮率：%.2f%%"), rs.hand_count > 0 ? rs.claim * 100 / static_cast<float>(rs.hand_count) : 0.0f);
    texts[10] = Common::format(__UTF8("自摸率：%.2f%%"), rs.win > 0 ? rs.self_drawn * 100 / static_cast<float>(rs.win) : 0.0f);
    texts[11] = Common::format(__UTF8("平均和牌番：%.2f"), rs.win > 0 ? rs.win_fan / static_cast<float>(rs.win) : 0.0f);
    texts[12] = Common::format(__UTF8("平均点炮番：%.2f"), rs.claim > 0 ? rs.claim_fan / static_cast<float>(rs.claim) : 0.0f);
    texts[13] = Common::format(__UTF8("和牌最大番：%hu"), rs.max_fan);
    texts[14] = Common::format(__UTF8("统计局数：%u"), rs.competition_count);
    texts[15] = Common::format(__UTF8("有效盘数：%u"), rs.hand_count);

    const float labelWidth = width * 0.5f - 4.0f;

    Label *label = Label::createWithSystemFont(texts[14], "Arail", 10);
    label->setTextColor(C4B_GRAY);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(2.0f, 125.0f));
    rootNode->addChild(label);
    cw::scaleLabelToFitWidth(label, labelWidth);

    label = Label::createWithSystemFont(texts[15], "Arail", 10);
    label->setTextColor(C4B_GRAY);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(2.0f + width * 0.5f, 125.0f));
    rootNode->addChild(label);
    cw::scaleLabelToFitWidth(label, labelWidth);

    for (int i = 0; i < 4; ++i) {
        const float yPos = 105.0f - 15.0f * i;
        label = Label::createWithSystemFont(texts[i], "Arail", 10);
        label->setTextColor(C4B_GRAY);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(2.0f, yPos));
        rootNode->addChild(label);
        cw::scaleLabelToFitWidth(label, labelWidth);

        label = Label::createWithSystemFont(texts[4 + i], "Arail", 10);
        label->setTextColor(C4B_GRAY);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(2.0f + width * 0.5f, yPos));
        rootNode->addChild(label);
        cw::scaleLabelToFitWidth(label, labelWidth);
    }

    for (int i = 0; i < 3; ++i) {
        const float yPos = 40.0f - 15.0f * i;
        label = Label::createWithSystemFont(texts[8 + i], "Arail", 10);
        label->setTextColor(C4B_GRAY);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(2.0f, yPos));
        rootNode->addChild(label);
        cw::scaleLabelToFitWidth(label, labelWidth);

        label = Label::createWithSystemFont(texts[11 + i], "Arail", 10);
        label->setTextColor(C4B_GRAY);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(2.0f + width * 0.5f, yPos));
        rootNode->addChild(label);
        cw::scaleLabelToFitWidth(label, labelWidth);
    }

    return rootNode;
}

namespace {

    class SummaryTableScene : public BaseScene, cw::TableViewDelegate {
    private:
        const std::vector<FilterIndex> *_filterIndices;
        std::vector<std::string> _timeTexts;
        std::vector<int8_t> _currentFlags;

        Label *_countLabel = nullptr;

    public:
        CREATE_FUNC_WITH_PARAM_1(SummaryTableScene, const std::vector<FilterIndex> *, filterIndices);

        bool init(const std::vector<FilterIndex> *filterIndices);

    private:
        virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
        virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
        virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

        void onRadioButtonGroup(ui::RadioButton *radioButton, int index, ui::RadioButtonGroup::EventType event);

        void refreshCountLabel();

        void onSummaryButton(cocos2d::Ref *sender);
    };

    bool SummaryTableScene::init(const std::vector<FilterIndex> *filterIndices) {
        if (UNLIKELY(!BaseScene::initWithTitle(__UTF8("个人汇总")))) {
            return false;
        }

        _currentFlags.assign(g_records.size(), -1);

        _filterIndices = filterIndices;
        _timeTexts.reserve(filterIndices->size());
        std::transform(filterIndices->begin(), filterIndices->end(), std::back_inserter(_timeTexts), [](const FilterIndex &data) {
            const Record &record = g_records[data.real_index];
            return formatTime(record.start_time, record.end_time);
        });

        Size visibleSize = Director::getInstance()->getVisibleSize();
        Vec2 origin = Director::getInstance()->getVisibleOrigin();

        Label *label = Label::createWithSystemFont("", "Arail", 10);
        label->setTextColor(C4B_GRAY);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(origin.x + 5.0f, origin.y + 15.0f));
        this->addChild(label);
        _countLabel = label;

        ui::Button *button = ui::Button::create("source_material/btn_square_disabled.png", "source_material/btn_square_selected.png");
        this->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(50.0f, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText(__UTF8("取消"));
        button->setPosition(Vec2(origin.x + visibleSize.width - 85.0f, origin.y +15.0f));
        button->addClickEventListener([](Ref *) { Director::getInstance()->popScene(); });

        button = UICommon::createButton();
        this->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(50.0f, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText(__UTF8("汇总"));
        button->setPosition(Vec2(origin.x + visibleSize.width - 30.0f, origin.y +15.0f));
        button->addClickEventListener(std::bind(&SummaryTableScene::onSummaryButton, this, std::placeholders::_1));

        // 表格
        cw::TableView *tableView = cw::TableView::create();
        tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
        tableView->setScrollBarPositionFromCorner(Vec2(2.0f, 2.0f));
        tableView->setScrollBarWidth(4.0f);
        tableView->setScrollBarOpacity(0x99);
        tableView->setContentSize(Size(visibleSize.width - 5.0f, visibleSize.height - 65.0f));
        tableView->setDelegate(this);
        tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

        tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
        tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f));
        tableView->reloadData();
        this->addChild(tableView);

        refreshCountLabel();

        return true;
    }

    void SummaryTableScene::refreshCountLabel() {
        char str[128];
        snprintf(str, sizeof(str), __UTF8("已选择：%") __UTF8(PRIzu) __UTF8("/%") __UTF8(PRIzu),
            _currentFlags.size() - std::count(_currentFlags.begin(), _currentFlags.end(), -1),
            _filterIndices->size());
        _countLabel->setString(str);
    }

    ssize_t SummaryTableScene::numberOfCellsInTableView(cw::TableView *) {
        return _filterIndices->size();
    }

    float SummaryTableScene::tableCellSizeForIndex(cw::TableView *, ssize_t) {
        return 50.0f;
    }

    cw::TableViewCell *SummaryTableScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
        typedef cw::TableViewCellEx<Label *, Label *, ui::RadioButtonGroup *, std::array<ui::RadioButton *, 4>, std::array<Label *, 4>, std::array<LayerColor *, 2> > CustomCell;
        CustomCell *cell = (CustomCell *)table->dequeueCell();

        const float cellWidth = table->getContentSize().width;

        if (cell == nullptr) {
            cell = CustomCell::create();

            CustomCell::ExtDataType &ext = cell->getExtData();
            Label *&titleLabel = std::get<0>(ext);
            Label *&timeLabel = std::get<1>(ext);
            ui::RadioButtonGroup *&radioGroup = std::get<2>(ext);
            ui::RadioButton **radioButtons = std::get<3>(ext).data();
            Label **labels = std::get<4>(ext).data();
            LayerColor **layerColors = std::get<5>(ext).data();

            // 背景色
            layerColors[0] = LayerColor::create(Color4B(0x10, 0x10, 0x10, 0x10), cellWidth, 50.0f);
            cell->addChild(layerColors[0]);

            layerColors[1] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), cellWidth, 50.0f);
            cell->addChild(layerColors[1]);

            // 标题
            Label *label = Label::createWithSystemFont("", "Arail", 10);
            label->setTextColor(C4B_BLACK);
            cell->addChild(label);
            label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
            label->setPosition(Vec2(2.0f, 43.0f));
            titleLabel = label;

            // 时间
            label = Label::createWithSystemFont("", "Arail", 10);
            label->setTextColor(C4B_GRAY);
            cell->addChild(label);
            label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
            label->setPosition(Vec2(2.0f, 28.0f));
            timeLabel = label;

            // 4个RadioButton和人名文本
            radioGroup = ui::RadioButtonGroup::create();
            cell->addChild(radioGroup);
            radioGroup->setAllowedNoSelection(true);
            for (int i = 0; i < 4; ++i) {
                ui::RadioButton *radioButton = UICommon::createRadioButton();
                radioButton->setZoomScale(0.0f);
                radioButton->ignoreContentAdaptWithSize(false);
                radioButton->setContentSize(Size(15.0f, 15.0f));
                radioButton->setPosition(Vec2(cellWidth * 0.25f * i + 10.0f, 10.0f));
                cell->addChild(radioButton);
                radioButtons[i] = radioButton;

                radioGroup->addRadioButton(radioButton);

                label = Label::createWithSystemFont("", "Arail", 10);
                cell->addChild(label);
                label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
                label->setPosition(Vec2(cellWidth * 0.25f * i + 20.0f, 10.0f));
                labels[i] = label;
            }
            radioGroup->addEventListener(std::bind(&SummaryTableScene::onRadioButtonGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

            // 清除按钮
            ui::Button *button = UICommon::createButton();
            cell->addChild(button);
            button->setScale9Enabled(true);
            button->setContentSize(Size(30.0f, 15.0f));
            button->setTitleFontSize(10);
            button->setTitleText(__UTF8("清除"));
            button->setPosition(Vec2(cellWidth - 17.0f, 40.0f));
            button->addClickEventListener([this, radioGroup](Ref *) {
                // 如果有选中，则清空
                int selectedButton = radioGroup->getSelectedButtonIndex();
                if (selectedButton != -1) {
                    ssize_t idx = reinterpret_cast<ssize_t>(radioGroup->getRadioButtonByIndex(selectedButton)->getUserData());
                    _currentFlags[idx] = -1;
                    radioGroup->setSelectedButton(nullptr);
                    refreshCountLabel();
                }
            });
        }

        const CustomCell::ExtDataType &ext = cell->getExtData();
        Label *titleLabel = std::get<0>(ext);
        Label *timeLabel = std::get<1>(ext);
        ui::RadioButtonGroup *radioGroup = std::get<2>(ext);
        ui::RadioButton *const *radioButtons = std::get<3>(ext).data();
        Label *const *labels = std::get<4>(ext).data();
        LayerColor *const *layerColors = std::get<5>(ext).data();

        layerColors[0]->setVisible((idx & 1) == 0);
        layerColors[1]->setVisible((idx & 1) != 0);

        const FilterIndex &data = _filterIndices->at(idx);
        size_t realIdx = data.real_index;

        const Record &record = g_records[realIdx];
        bool finished = record.end_time != 0;

        // 标题
        titleLabel->setString(formatTitle(record));
        cw::scaleLabelToFitWidth(titleLabel, cellWidth - 35.0f - 2.0f);

        // 时间
        timeLabel->setString(_timeTexts[idx]);
        cw::scaleLabelToFitWidth(timeLabel, cellWidth - 35.0f - 2.0f);

        // 人名文本
        for (int i = 0; i < 4; ++i) {
            ui::RadioButton *radioButton = radioButtons[i];
            Label *label = labels[i];
            radioButton->setEnabled(finished);
            radioButton->setUserData(reinterpret_cast<void *>(realIdx));
            label->setString(record.name[i]);
            label->setTextColor(((1U << i) & data.player_flag) ? C4B_RED : C4B_GRAY);
            cw::scaleLabelToFitWidth(label, cellWidth * 0.25f - 20.0f - 2.0f);
        }

        // 设置选中
        int8_t flag = _currentFlags[realIdx];
        radioGroup->setSelectedButton(flag == -1 ? nullptr : radioButtons[flag]);

        return cell;
    }

    void SummaryTableScene::onRadioButtonGroup(ui::RadioButton *radioButton, int index, ui::RadioButtonGroup::EventType) {
        if (radioButton == nullptr) {
            return;
        }

        ssize_t idx = reinterpret_cast<ssize_t>(radioButton->getUserData());
        _currentFlags[idx] = static_cast<int8_t>(index);
        refreshCountLabel();
    }

    void SummaryTableScene::onSummaryButton(cocos2d::Ref *) {
        RecordsStatistic rs;
        SummarizeRecords(_currentFlags, g_records, &rs);
        if (rs.competition_count == 0) {
            Toast::makeText(this, __UTF8("请选择要汇总的对局"), Toast::Duration::LENGTH_LONG)->show();
            return;
        }

        Node *node = createStatisticNode(rs);
        AlertDialog::Builder(this)
            .setTitle(__UTF8("汇总结果"))
            .setContentNode(node)
            .setPositiveButton(__UTF8("确定"), nullptr)
            .create()->show();
    }
}

void RecordHistoryScene::switchToSummary() {
    SummaryTableScene *scene = SummaryTableScene::create(&_filterIndices);
    Director::getInstance()->pushScene(scene);
}

namespace {

    class MultiSelectTableScene : public BaseScene, cw::TableViewDelegate {
    private:
        const std::vector<RecordTexts> *_texts;
        const std::vector<FilterIndex> *_filterIndices;
        std::vector<bool> _currentFlags;

        Label *_countLabel = nullptr;

    public:
        const std::vector<bool> &getCurrentFlags() { return _currentFlags; }

        CREATE_FUNC_WITH_PARAM_4(MultiSelectTableScene, const std::vector<RecordTexts> *, texts, const std::vector<FilterIndex> *, filterIndices, std::string &&, title, std::string &&, buttonTitle);

        bool init(const std::vector<RecordTexts> * texts, const std::vector<FilterIndex> *filterIndices, std::string &&title, std::string &&buttonTitle);

        typedef std::function<void (MultiSelectTableScene *)> PositiveCallback;
        void setPositiveCallback(PositiveCallback &&callback) { _positiveCallback.swap(callback); }

    private:
        virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
        virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
        virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

        void refreshCountLabel();

        PositiveCallback _positiveCallback;
    };

    bool MultiSelectTableScene::init(const std::vector<RecordTexts> * texts, const std::vector<FilterIndex> *filterIndices, std::string &&title, std::string &&buttonTitle) {
        if (UNLIKELY(!BaseScene::initWithTitle(title))) {
            return false;
        }

        _currentFlags.assign(g_records.size(), 0);

        _texts = texts;
        _filterIndices = filterIndices;

        Size visibleSize = Director::getInstance()->getVisibleSize();
        Vec2 origin = Director::getInstance()->getVisibleOrigin();

        Label *label = Label::createWithSystemFont("", "Arail", 10);
        label->setTextColor(C4B_GRAY);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_RIGHT);
        label->setPosition(Vec2(origin.x + visibleSize.width - 115.0f, origin.y + 15.0f));
        this->addChild(label);
        _countLabel = label;

        ui::Button *button = ui::Button::create("source_material/btn_square_disabled.png", "source_material/btn_square_selected.png");
        this->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(50.0f, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText(__UTF8("取消"));
        button->setPosition(Vec2(origin.x + visibleSize.width - 85.0f, origin.y + 15.0f));
        button->addClickEventListener([](Ref *) { Director::getInstance()->popScene(); });

        button = UICommon::createButton();
        this->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(50.0f, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText(buttonTitle);
        button->setPosition(Vec2(origin.x + visibleSize.width - 30.0f, origin.y + 15.0f));
        button->addClickEventListener([this](Ref *) { if (_positiveCallback) _positiveCallback(this); });

        // 表格
        cw::TableView *tableView = cw::TableView::create();
        tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
        tableView->setScrollBarPositionFromCorner(Vec2(2.0f, 2.0f));
        tableView->setScrollBarWidth(4.0f);
        tableView->setScrollBarOpacity(0x99);
        tableView->setContentSize(Size(visibleSize.width - 5.0f, visibleSize.height - 65.0f));
        tableView->setDelegate(this);
        tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

        tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
        tableView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f));
        tableView->reloadData();
        this->addChild(tableView);

        refreshCountLabel();

        ui::CheckBox *checkBox = UICommon::createCheckBox();
        this->addChild(checkBox);
        checkBox->setZoomScale(0.0f);
        checkBox->ignoreContentAdaptWithSize(false);
        checkBox->setContentSize(Size(15.0f, 15.0f));
        checkBox->setPosition(Vec2(origin.x + 12.5f, origin.y + 15.0f));
        checkBox->addEventListener([this, tableView](Ref *sender, ui::CheckBox::EventType) {
            ui::CheckBox *checkBox = (ui::CheckBox *)sender;
            _currentFlags.assign(_currentFlags.size(), false);
            if (checkBox->isSelected()) {
                for (size_t i = 0, cnt = _filterIndices->size(); i < cnt; ++i) {
                    _currentFlags[_filterIndices->at(i).real_index] = true;
                }
            }
            tableView->reloadDataInplacement();
            refreshCountLabel();
        });

        label = Label::createWithSystemFont(__UTF8("全选"), "Arail", 10);
        label->setTextColor(C4B_GRAY);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(origin.x + 25.0f, origin.y + 15.0f));
        this->addChild(label);

        return true;
    }

    void MultiSelectTableScene::refreshCountLabel() {
        char str[128];
        snprintf(str, sizeof(str), __UTF8("已选择：%") __UTF8(PRIzu) __UTF8("/%") __UTF8(PRIzu),
            _currentFlags.size() - std::count(_currentFlags.begin(), _currentFlags.end(), false),
            _filterIndices->size());
        _countLabel->setString(str);
    }

    ssize_t MultiSelectTableScene::numberOfCellsInTableView(cw::TableView *) {
        return _filterIndices->size();
    }

    float MultiSelectTableScene::tableCellSizeForIndex(cw::TableView *, ssize_t) {
        return 65.0f;
    }

    cw::TableViewCell *MultiSelectTableScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
        typedef cw::TableViewCellEx<Label *, Label *, std::array<Label *, 4>, ui::CheckBox *, std::array<LayerColor *, 2> > CustomCell;
        CustomCell *cell = (CustomCell *)table->dequeueCell();

        const float cellWidth = table->getContentSize().width;

        const float nameWidth = (cellWidth - 25.0f) * 0.5f;

        if (cell == nullptr) {
            cell = CustomCell::create();

            CustomCell::ExtDataType &ext = cell->getExtData();
            Label *&titleLabel = std::get<0>(ext);
            Label *&timeLabel = std::get<1>(ext);
            Label **playerLabels = std::get<2>(ext).data();
            ui::CheckBox *&checkBox = std::get<3>(ext);
            LayerColor **layerColors = std::get<4>(ext).data();

            // 背景色
            layerColors[0] = LayerColor::create(Color4B(0x10, 0x10, 0x10, 0x10), cellWidth, 65.0f);
            cell->addChild(layerColors[0]);

            layerColors[1] = LayerColor::create(Color4B(0xC0, 0xC0, 0xC0, 0x10), cellWidth, 65.0f);
            cell->addChild(layerColors[1]);

            // 标题
            Label *label = Label::createWithSystemFont("", "Arail", 10);
            label->setTextColor(C4B_BLACK);
            cell->addChild(label);
            label->setPosition(Vec2(25.0f, 55.0f));
            label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
            titleLabel = label;

            // 时间
            label = Label::createWithSystemFont("", "Arail", 10);
            label->setTextColor(C4B_GRAY);
            cell->addChild(label);
            label->setPosition(Vec2(25.0f, 40.0f));
            label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
            timeLabel = label;

            // 四名选手
            for (int i = 0; i < 4; ++i) {
                label = Label::createWithSystemFont("", "Arail", 10);
                cell->addChild(label);
                label->setPosition(Vec2(25.0f + (i & 1) * nameWidth, 23.0f - (i >> 1) * 15.0f));
                label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
                playerLabels[i] = label;
            }

            checkBox = UICommon::createCheckBox();
            cell->addChild(checkBox);
            checkBox->setZoomScale(0.0f);
            checkBox->ignoreContentAdaptWithSize(false);
            checkBox->setContentSize(Size(15.0f, 15.0f));
            checkBox->setPosition(Vec2(10.0f, 32.5f));
            checkBox->addEventListener([this](Ref *sender, ui::CheckBox::EventType) {
                ui::CheckBox *checkBox = (ui::CheckBox *)sender;
                ssize_t idx = reinterpret_cast<ssize_t>(checkBox->getUserData());
                _currentFlags[idx] = checkBox->isSelected();
                refreshCountLabel();
            });

            cell->setContentSize(Size(cellWidth, 65.0f));
            cell->addClickEventListener([this](Ref *sender) {
                CustomCell *cell = (CustomCell *)sender;
                ui::CheckBox *checkBox = std::get<3>(cell->getExtData());
                ssize_t idx = reinterpret_cast<ssize_t>(checkBox->getUserData());
                checkBox->setSelected(_currentFlags[idx] = !checkBox->isSelected());
                refreshCountLabel();
            });
        }

        const CustomCell::ExtDataType &ext = cell->getExtData();
        Label *titleLabel = std::get<0>(ext);
        Label *timeLabel = std::get<1>(ext);
        Label *const *playerLabels = std::get<2>(ext).data();
        ui::CheckBox *checkBox = std::get<3>(ext);
        LayerColor *const *layerColors = std::get<4>(ext).data();

        layerColors[0]->setVisible((idx & 1) == 0);
        layerColors[1]->setVisible((idx & 1) != 0);

        const FilterIndex &data = _filterIndices->at(idx);
        size_t realIdx = data.real_index;

        const RecordTexts &texts = _texts->at(realIdx);
        titleLabel->setString(texts.title);
        cw::scaleLabelToFitWidth(titleLabel, cellWidth - 30.0f);

        timeLabel->setString(texts.time);
        cw::scaleLabelToFitWidth(timeLabel, cellWidth - 30.0f);

        uint8_t flag = data.player_flag;
        for (int i = 0; i < 4; ++i) {
            Label *label = playerLabels[i];
            label->setString(texts.players[i]);
            label->setTextColor(((1U << texts.seats[i]) & flag) ? C4B_RED : C4B_GRAY);
            cw::scaleLabelToFitWidth(label, nameWidth - 4.0f);
        }

        const Record &record = *texts.source;
        bool finished = record.end_time != 0;

        // 设置选中
        checkBox->setUserData(reinterpret_cast<void *>(realIdx));
        checkBox->setSelected(_currentFlags[realIdx]);
        checkBox->setEnabled(finished);

        cell->setTouchEnabled(finished);

        return cell;
    }
}

void RecordHistoryScene::switchToBatchDelete() {
    MultiSelectTableScene *scene = MultiSelectTableScene::create(&_recordTexts, &_filterIndices, __UTF8("批量删除"), __UTF8("删除"));
    scene->setPositiveCallback([this](MultiSelectTableScene *scene) {
        const std::vector<bool> &currentFlags = scene->getCurrentFlags();

        if (std::none_of(currentFlags.begin(), currentFlags.end(), [](bool f) { return f; })) {
            Toast::makeText(scene, __UTF8("请选择要删除的对局"), Toast::Duration::LENGTH_LONG)->show();
            return;
        }

        AlertDialog::Builder(scene)
            .setTitle(__UTF8("删除记录"))
            .setMessage(__UTF8("删除后无法找回，确认删除？"))
            .setNegativeButton(__UTF8("取消"), nullptr)
            .setPositiveButton(__UTF8("确定"), [this, scene](AlertDialog *, int) {
            const std::vector<bool> &currentFlags = scene->getCurrentFlags();
            for (size_t i = currentFlags.size(); i-- > 0; ) {
                if (currentFlags[i]) {
                    g_records.erase(g_records.begin() + i);
                }
            }
            saveRecordsAndRefresh();

            Director::getInstance()->popScene();

            return true;
        }).create()->show();
    });

    Director::getInstance()->pushScene(scene);
}

void RecordHistoryScene::showTransmissionAlert() {
    const float limitWidth = AlertDialog::maxWidth();

    Node *rootNode = Node::create();

    Label *label = Label::createWithSystemFont(
        __UTF8("说明：\n")
        __UTF8("1. 点对点传输需两台设备连接同一WiFi或者两台设备通过热点连接，热点连接不耗费手机流量\n")
        __UTF8("2. 传输前请确认是否开启了网络权限"),
        "Arial", 10, Size(limitWidth - 4.0f, 0.0f));
    label->setLineSpacing(2.0f);
    rootNode->addChild(label);
    label->setTextColor(C4B_BLACK);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_BOTTOM);
    label->setPosition(Vec2(limitWidth * 0.5f, 50.0f));

    rootNode->setContentSize(Size(limitWidth, label->getContentSize().height + 55.0f));

    ui::Button *button1 = UICommon::createButton();
    rootNode->addChild(button1);
    button1->setScale9Enabled(true);
    button1->setContentSize(Size(55.0f, 20.0f));
    button1->setTitleFontSize(12);
    button1->setTitleText(__UTF8("发送"));
    button1->setPosition(Vec2((limitWidth - 110.0f) / 3.0f + 27.5f, 25.0f));
    button1->setEnabled(g_hasLoaded && !g_records.empty());

    ui::Button *button2 = UICommon::createButton();
    rootNode->addChild(button2);
    button2->setScale9Enabled(true);
    button2->setContentSize(Size(55.0f, 20.0f));
    button2->setTitleFontSize(12);
    button2->setTitleText(__UTF8("接收"));
    button2->setPosition(Vec2(limitWidth - button1->getPositionX(), 25.0f));

    AlertDialog *dlg = AlertDialog::Builder(this)
        .setTitle(__UTF8("点对点传输"))
        .setContentNode(rootNode)
        .create();
    dlg->show();

    button1->addClickEventListener([this, dlg](Ref *) {
        AlertDialog::Builder(this)
            .setTitle(__UTF8("点对点传输——发送"))
            .setMessage(__UTF8("请选择需要传输的记录"))
            .setNegativeButton(__UTF8("取消"), nullptr)
            .setPositiveButton(__UTF8("确定"), [this](AlertDialog *, int) {

            MultiSelectTableScene *scene = MultiSelectTableScene::create(&_recordTexts, &_filterIndices, __UTF8("点对点传输——发送"), __UTF8("确定"));
            scene->setPositiveCallback([this](MultiSelectTableScene *scene) {
                std::vector<bool> currentFlags = scene->getCurrentFlags();

                if (std::none_of(currentFlags.begin(), currentFlags.end(), [](bool f) { return f; })) {
                    Toast::makeText(scene, __UTF8("请选择要传输的对局"), Toast::Duration::LENGTH_LONG)->show();
                    return;
                }

                Director::getInstance()->popScene();

                showSendAlert(std::move(currentFlags));
            });
            Director::getInstance()->pushScene(scene);

            return true;
        }).create()->show();

        dlg->dismiss();
    });
    button2->addClickEventListener([this, dlg](Ref *) {
        showRecvAlert();
        dlg->dismiss();
    });
}

void RecordHistoryScene::showSendAlert(std::vector<bool> selectFlags) {
    const float limitWidth = AlertDialog::maxWidth();

    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(limitWidth, 100.0f));

    Label *label = Label::createWithSystemFont(__UTF8("请在另一台设备上输入以下IP地址"), "Arial", 10);
    cw::scaleLabelToFitWidth(label, limitWidth - 4.0f);
    rootNode->addChild(label);
    label->setTextColor(C4B_BLACK);
    label->setPosition(Vec2(limitWidth * 0.5f, 90.0f));

    auto socketSender = std::make_shared<p2p::Sender>();
    rootNode->setOnExitCallback([socketSender]() { socketSender->quit(); });

    std::string addr = socketSender->prepare();

    label = Label::createWithSystemFont(addr, "Arial", 16);
    rootNode->addChild(label);
    label->setTextColor(C4B_RED);
    label->setPosition(Vec2(limitWidth * 0.5f, 70.0f));

    ui::Button *button = UICommon::createButton();
    rootNode->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText(__UTF8("发送"));
    button->setPosition(Vec2(limitWidth * 0.5f, 35.0f));
    button->setEnabled(false);

    size_t totalCnt = std::count(selectFlags.begin(), selectFlags.end(), true);

    label = Label::createWithSystemFont(Common::format(__UTF8("等待连接，即将传输%") __UTF8(PRIzu) __UTF8("条记录"), totalCnt), "Arial", 10);
    rootNode->addChild(label);
    label->setTextColor(C4B_BLACK);
    label->setPosition(Vec2(limitWidth * 0.5f, 10.0f));

    auto isSending = std::make_shared<bool>();

    AlertDialog *dialog = AlertDialog::Builder(this)
        .setTitle(__UTF8("点对点传输——发送"))
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setNegativeButton(__UTF8("退出"), [this, isSending, socketSender](AlertDialog *, int) {
        if (*isSending) {
            AlertDialog::Builder(this)
                .setTitle(__UTF8("点对点传输——发送"))
                .setMessage(__UTF8("确定要终止传输？"))
                .setNegativeButton(__UTF8("继续"), nullptr)
                .setPositiveButton(__UTF8("终止"), [socketSender](AlertDialog *, int) {
                socketSender->quit();
                return true;
            }).create()->show();

            return false;
        }
        return true;
    }).create();
    dialog->show();

    auto dialogStrong = makeRef(dialog);
    auto acceptRet = std::make_shared<bool>();

    // 等待客户端连接
    AsyncTaskPool::getInstance()->enqueue(AsyncTaskPool::TaskType::TASK_IO, [dialogStrong, totalCnt, acceptRet, label, button](void *) {
        if (UNLIKELY(!dialogStrong->isRunning())) {
            return;
        }

        if (*acceptRet) {
            label->setString(Common::format(__UTF8("连接成功，可传输%") __UTF8(PRIzu) __UTF8("条记录"), totalCnt));
            button->setEnabled(true);
        }
        else {
            label->setString(__UTF8("连接失败，请退出后重试"));
        }
    }, nullptr, [socketSender, acceptRet]() { *acceptRet = socketSender->accept(); });

    auto selectFlagsPtr = std::make_shared<std::vector<bool> >(std::move(selectFlags));
    button->addClickEventListener([this, socketSender, isSending, dialog, label, selectFlagsPtr, totalCnt](Ref *sender) {
        ui::Button *button = (ui::Button *)sender;
        button->setEnabled(false);
        label->setString(__UTF8("数据传输中，请勿退出程序"));

        auto thiz = makeRef(this);
        auto dialogStrong = makeRef(dialog);
        auto sendRet = std::make_shared<size_t>();

        // 发送
        AsyncTaskPool::getInstance()->enqueue(AsyncTaskPool::TaskType::TASK_IO, [thiz, dialogStrong, sendRet, totalCnt, label, button](void *) {
            if (LIKELY(dialogStrong->isRunning())) {
                if (*sendRet == 0) {
                    label->setString(__UTF8("数据传输失败，请退出后重试"));
                    button->setEnabled(true);
                }
                else {
                    dialogStrong->dismiss();
                    if (LIKELY(thiz->isRunning())) {
                        AlertDialog::Builder(thiz.get())
                            .setTitle(__UTF8("提示"))
                            .setMessage(Common::format(__UTF8("数据传输完毕，共%") __UTF8(PRIzu) __UTF8("/%") __UTF8(PRIzu) __UTF8("条记录"), *sendRet, totalCnt))
                            .setPositiveButton(__UTF8("确定"), nullptr)
                            .create()->show();
                    }
                }
            }
        }, nullptr, [dialogStrong, socketSender, isSending, sendRet, selectFlagsPtr, totalCnt, label]() {
            *isSending = true;

            const std::vector<bool> &selectFlags = *selectFlagsPtr;

            // 先发送个数
            char str[16];
            int len = 1 + snprintf(str, sizeof(str), "%" PRIzu, totalCnt);
            ssize_t ret = socketSender->send(str, len);
            if (ret != len) {
                *sendRet = 0;
                *isSending = false;
                return;
            }

            // 再逐个记录发送
            size_t currentCnt = 0;
            std::vector<char> buf;
            for (size_t i = 0, cnt = selectFlags.size(); i < cnt; ++i) {
                if (!selectFlags[i]) {
                    continue;
                }

                StringifyRecord(buf, g_records[i]);
                if (buf.back() != '\0') buf.push_back('\0');  // 以'\0'作为一条记录的结束

                ret = socketSender->send(buf.data(), static_cast<int>(buf.size()));
                if (ret <= 0 || static_cast<size_t>(ret) != buf.size()) {
                    break;
                }
                ++currentCnt;

                Director::getInstance()->getScheduler()->performFunctionInCocosThread([dialogStrong, label, currentCnt, totalCnt]() {
                    if (UNLIKELY(!dialogStrong->isRunning())) {
                        return;
                    }
                    label->setString(Common::format(__UTF8("正在传输中，请勿退出程序。%") __UTF8(PRIzu) __UTF8("/%") __UTF8(PRIzu), currentCnt, totalCnt));
                });
            }

            socketSender->quit();
            *sendRet = currentCnt;

            *isSending = false;
        });
    });
}

void RecordHistoryScene::showRecvAlert() {
    const float limitWidth = AlertDialog::maxWidth();

    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(limitWidth, 100.0f));

    Label *label = Label::createWithSystemFont(__UTF8("请输入另一台设备的IP地址"), "Arial", 10);
    cw::scaleLabelToFitWidth(label, limitWidth - 4.0f);
    rootNode->addChild(label);
    label->setTextColor(C4B_BLACK);
    label->setPosition(Vec2(limitWidth * 0.5f, 90.0f));

    ui::EditBox *editBox = UICommon::createEditBox(Size(200, 20));
    editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::NEXT);
    editBox->setFontColor(C4B_BLACK);
    editBox->setFontSize(16);
    editBox->setMaxLength(22);
    rootNode->addChild(editBox);
    editBox->setPosition(Vec2(limitWidth * 0.5f, 65.0f));

    auto socketReceiver = std::make_shared<p2p::Reciever>();
    rootNode->setOnExitCallback([socketReceiver]() { socketReceiver->quit(); });

    ui::Button *button = UICommon::createButton();
    rootNode->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText(__UTF8("连接"));
    button->setPosition(Vec2(limitWidth * 0.5f, 35.0f));

    label = Label::createWithSystemFont(__UTF8("等待连接，请从另一台设备上获取IP地址"), "Arial", 10);
    rootNode->addChild(label);
    label->setTextColor(C4B_BLACK);
    label->setPosition(Vec2(limitWidth * 0.5f, 10.0f));

    auto isReceiving = std::make_shared<bool>();

    AlertDialog *dialog = AlertDialog::Builder(this)
        .setTitle(__UTF8("点对点传输——接收"))
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setNegativeButton(__UTF8("退出"), [this, socketReceiver, isReceiving](AlertDialog *, int) {
        if (*isReceiving) {
            AlertDialog::Builder(this)
                .setTitle(__UTF8("点对点传输——接收"))
                .setMessage(__UTF8("确定要终止传输？"))
                .setNegativeButton(__UTF8("继续"), nullptr)
                .setPositiveButton(__UTF8("终止"), [socketReceiver](AlertDialog *, int) {
                socketReceiver->quit();
                return true;
            }).create()->show();

            return false;
        }

        return true;
    }).create();
    dialog->show();

    button->addClickEventListener([this, dialog, editBox, label, socketReceiver, isReceiving](Ref *sender) {
        const char *text = editBox->getText();
        if (text == nullptr || *text == '\0') {
            return;
        }

        std::string address = text;

        label->setString(__UTF8("正在连接，请勿退出程序"));
        ui::Button *button = (ui::Button *)sender;
        button->setEnabled(false);

        auto thiz = makeRef(this);
        auto dialogStrong = makeRef(dialog);

        auto connectRet = std::make_shared<bool>();

        // 连接服务器
        AsyncTaskPool::getInstance()->enqueue(AsyncTaskPool::TaskType::TASK_IO, [thiz, dialogStrong, isReceiving, connectRet, label, button, socketReceiver](void *) {
            if (UNLIKELY(!dialogStrong->isRunning())) {
                return;
            }

            if (!*connectRet) {
                label->setString(__UTF8("连接失败"));
                return;
            }

            label->setString(__UTF8("等待传输数据，请在另一设备上点击「发送」"));

            auto recvRet = std::make_shared<size_t>();
            auto destCnt = std::make_shared<size_t>();

            // 接收数据
            AsyncTaskPool::getInstance()->enqueue(AsyncTaskPool::TaskType::TASK_IO, [thiz, dialogStrong, recvRet, label, button, destCnt](void *) {
                if (*recvRet > 0) {
                    if (LIKELY(dialogStrong->isRunning())) {
                        dialogStrong->dismiss();
                        if (LIKELY(thiz->isRunning())) {
                            AlertDialog::Builder(thiz.get())
                                .setTitle(__UTF8("提示"))
                                .setMessage(Common::format(__UTF8("数据传输完毕，共%") __UTF8(PRIzu) __UTF8("/%") __UTF8(PRIzu) __UTF8("条记录"), *recvRet, *destCnt))
                                .setPositiveButton(__UTF8("确定"), nullptr)
                                .create()->show();
                        }
                    }

                    if (LIKELY(thiz->isRunning())) {
                        thiz->updateRecordTexts();
                        thiz->refresh();
                    }
                }
                else {
                    if (LIKELY(dialogStrong->isRunning())) {
                        label->setString(__UTF8("正在传输失败，请退出后重试"));
                        button->setEnabled(false);
                    }
                }
            }, nullptr, [dialogStrong, label, socketReceiver, destCnt, recvRet, isReceiving]() {
                std::vector<char> str;
                char buf[1024];

                size_t cnt = 0;

                *isReceiving = true;

                do {
                    ssize_t ret = socketReceiver->recv(buf, sizeof(buf));
                    if (ret <= 0) {
                        break;
                    }

                    if (UNLIKELY(*destCnt == 0)) {  // 第一个是个数
                        sscanf(buf, "%" PRIzu, &*destCnt);

                        Director::getInstance()->getScheduler()->performFunctionInCocosThread([dialogStrong, label, destCnt]() {
                            if (UNLIKELY(!dialogStrong->isRunning())) {
                                return;
                            }

                            label->setString(Common::format(__UTF8("正在传输中，请勿退出程序。0/%") __UTF8(PRIzu), *destCnt));
                        });

                        char *p = std::find(buf, buf + ret, '\0');
                        if (p != buf + ret) {
                            str.reserve(str.size() + (buf + ret - p));
                            std::copy(p + 1, buf + ret, std::back_inserter(str));
                        }
                    }
                    else {  // 随后的是内容
                        str.reserve(str.size() + ret);
                        std::copy(buf, buf + ret, std::back_inserter(str));
                    }

                    // 遇到'\0'为一条记录结束
                    for (std::vector<char>::iterator p = std::find(str.begin(), str.end(), '\0');
                        p != str.end(); p = std::find(str.begin(), str.end(), '\0')) {
                        Record record;
                        ParseRecord(str.data(), record);
                        ModifyRecordInVector(g_records, &record);  // 更新即可
                        ++cnt;

                        Director::getInstance()->getScheduler()->performFunctionInCocosThread([dialogStrong, label, destCnt, cnt]() {
                            if (UNLIKELY(!dialogStrong->isRunning())) {
                                return;
                            }

                            label->setString(Common::format(__UTF8("正在传输中，请勿退出程序。%") __UTF8(PRIzu) __UTF8("/%") __UTF8(PRIzu), cnt, *destCnt));
                        });

                        str.erase(str.begin(), p + 1);
                    }
                } while (cnt < *destCnt);

                socketReceiver->quit();
                *recvRet = cnt;

                *isReceiving = false;

                // 保存
                saveRecords(g_records);
            });
        }, nullptr, [socketReceiver, address, connectRet]() { *connectRet = socketReceiver->connect(address.c_str()); });
    });
}

void RecordHistoryScene::onCellClicked(cocos2d::Ref *sender) {
    cw::TableViewCell *cell = (cw::TableViewCell *)sender;
    size_t idx = reinterpret_cast<size_t>(cell->getUserData());
    _viewCallback(&g_records[idx]);
}

void RecordHistoryScene::modifyRecord(const Record *record) {
    if (UNLIKELY(!g_hasLoaded)) {  // 如果当前没有加载过历史记录
        auto r = std::make_shared<Record>(*record);  // 复制当前记录
        auto records = std::make_shared<std::vector<Record> >();

        // 在子线程中加载、修改、保存，然后切换到主线程，覆盖整个历史记录
        AsyncTaskPool::getInstance()->enqueue(AsyncTaskPool::TaskType::TASK_IO,
            [records](void *) { g_records.swap(*records); }, nullptr,
            [r, records]() {
            loadRecords(*records);
            ModifyRecordInVector(*records, r.get());
            saveRecords(*records);
        });
    }
    else {  // 如果当前加载过历史记录
        // 直接修改
        ModifyRecordInVector(g_records, record);

        // 子线程中保存
        auto records = std::make_shared<std::vector<Record> >(g_records);
        AsyncTaskPool::getInstance()->enqueue(AsyncTaskPool::TaskType::TASK_IO, [records]() { saveRecords(*records); });
    }
}
