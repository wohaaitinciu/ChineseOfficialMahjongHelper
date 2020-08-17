#include "RecordScene.h"
#include <array>
#include "../mahjong-algorithm/fan_calculator.h"
#include "../UICommon.h"
#include "../UIColors.h"
#include "../widget/AlertDialog.h"
#include "../widget/Toast.h"
#include "../widget/CheckBoxScale9.h"
#include "../widget/TilePickWidget.h"
#include "../widget/ExtraInfoWidget.h"
#include "../mahjong-algorithm/stringify.h"

USING_NS_CC;

#define RECENT_FANS "recent_fans"
#define USE_FIXED_SEAT_ORDER "use_fixed_seat_order"
#define NEVER_NOTIFY_LITTLE_FAN "never_notify_little_fan"

// 8个常用番作为初始的「最近使用」
static mahjong::fan_t recentFans[8] = {
    mahjong::MIXED_SHIFTED_CHOWS,
    mahjong::MIXED_TRIPLE_CHOW,
    mahjong::MIXED_STRAIGHT,
    mahjong::ALL_TYPES,
    mahjong::PURE_STRAIGHT,
    mahjong::HALF_FLUSH,
    mahjong::ALL_PUNGS,
    mahjong::PURE_SHIFTED_CHOWS
};

static const mahjong::fan_t standardFans[mahjong::FAN_TABLE_SIZE] = {
    mahjong::FAN_NONE,

    mahjong::BIG_FOUR_WINDS,
    mahjong::BIG_THREE_DRAGONS,
    mahjong::ALL_GREEN,
    mahjong::NINE_GATES,
    mahjong::FOUR_KONGS,
    mahjong::SEVEN_SHIFTED_PAIRS,
    mahjong::THIRTEEN_ORPHANS,

    mahjong::ALL_TERMINALS,
    mahjong::LITTLE_FOUR_WINDS,
    mahjong::LITTLE_THREE_DRAGONS,
    mahjong::ALL_HONORS,
    mahjong::FOUR_CONCEALED_PUNGS,
    mahjong::PURE_TERMINAL_CHOWS,

    mahjong::QUADRUPLE_CHOW,
    mahjong::FOUR_PURE_SHIFTED_PUNGS,

    mahjong::FOUR_PURE_SHIFTED_CHOWS,
    mahjong::THREE_KONGS,
    mahjong::ALL_TERMINALS_AND_HONORS,

    mahjong::SEVEN_PAIRS,
    mahjong::GREATER_HONORS_AND_KNITTED_TILES,
    mahjong::ALL_EVEN_PUNGS,
    mahjong::FULL_FLUSH,
    mahjong::PURE_TRIPLE_CHOW,
    mahjong::PURE_SHIFTED_PUNGS,
    mahjong::UPPER_TILES,
    mahjong::MIDDLE_TILES,
    mahjong::LOWER_TILES,

    mahjong::PURE_STRAIGHT,
    mahjong::THREE_SUITED_TERMINAL_CHOWS,
    mahjong::PURE_SHIFTED_CHOWS,
    mahjong::ALL_FIVE,
    mahjong::TRIPLE_PUNG,
    mahjong::THREE_CONCEALED_PUNGS,

    mahjong::LESSER_HONORS_AND_KNITTED_TILES,
    mahjong::KNITTED_STRAIGHT,
    mahjong::UPPER_FOUR,
    mahjong::LOWER_FOUR,
    mahjong::BIG_THREE_WINDS,

    mahjong::MIXED_STRAIGHT,
    mahjong::REVERSIBLE_TILES,
    mahjong::MIXED_TRIPLE_CHOW,
    mahjong::MIXED_SHIFTED_PUNGS,
    mahjong::CHICKEN_HAND,
    mahjong::LAST_TILE_DRAW,
    mahjong::LAST_TILE_CLAIM,
    mahjong::OUT_WITH_REPLACEMENT_TILE,
    mahjong::ROBBING_THE_KONG,

    mahjong::ALL_PUNGS,
    mahjong::HALF_FLUSH,
    mahjong::MIXED_SHIFTED_CHOWS,
    mahjong::ALL_TYPES,
    mahjong::MELDED_HAND,
    mahjong::TWO_CONCEALED_KONGS,
    mahjong::TWO_DRAGONS_PUNGS,

    mahjong::OUTSIDE_HAND,
    mahjong::FULLY_CONCEALED_HAND,
    mahjong::TWO_MELDED_KONGS,
    mahjong::LAST_TILE
};

namespace {
    typedef struct {
        const char *const title;
        const mahjong::fan_t *const fans;
        unsigned count;
        const mahjong::fan_t first_fan;
    } CellDetail;
}

static CellDetail cellDetails[11] = {
    { __UTF8("最近使用"), recentFans, 8, mahjong::FAN_NONE },
    { __UTF8("4番"), &standardFans[mahjong::OUTSIDE_HAND], 4, mahjong::OUTSIDE_HAND },
    { __UTF8("6番"), &standardFans[mahjong::ALL_PUNGS], 7, mahjong::ALL_PUNGS },
    { __UTF8("8番"), &standardFans[mahjong::MIXED_STRAIGHT], 9, mahjong::MIXED_STRAIGHT },
    { __UTF8("12番"), &standardFans[mahjong::LESSER_HONORS_AND_KNITTED_TILES], 5, mahjong::LESSER_HONORS_AND_KNITTED_TILES },
    { __UTF8("16番"), &standardFans[mahjong::PURE_STRAIGHT], 6, mahjong::PURE_STRAIGHT },
    { __UTF8("24番"), &standardFans[mahjong::SEVEN_PAIRS], 9, mahjong::SEVEN_PAIRS },
    { __UTF8("32番"), &standardFans[mahjong::FOUR_PURE_SHIFTED_CHOWS], 3, mahjong::FOUR_PURE_SHIFTED_CHOWS },
    { __UTF8("48番"), &standardFans[mahjong::QUADRUPLE_CHOW], 2, mahjong::QUADRUPLE_CHOW },
    { __UTF8("64番"), &standardFans[mahjong::ALL_TERMINALS], 6, mahjong::ALL_TERMINALS },
    { __UTF8("88番"), &standardFans[mahjong::BIG_FOUR_WINDS], 7, mahjong::BIG_FOUR_WINDS }
};

static void loadRecentFans() {
    std::string str = UserDefault::getInstance()->getStringForKey(RECENT_FANS);
    if (str.empty()) {
        return;
    }

    int recentFansInt[8];
    if (8 == sscanf(str.c_str(), "%d %d %d %d %d %d %d %d",
        &recentFansInt[0], &recentFansInt[1], &recentFansInt[2], &recentFansInt[3],
        &recentFansInt[4], &recentFansInt[5], &recentFansInt[6], &recentFansInt[7])) {
        std::transform(std::begin(recentFansInt), std::end(recentFansInt), std::begin(recentFans), [](int fan) { return static_cast<mahjong::fan_t>(fan); });
    }
}

static void saveRecentFans() {
    UserDefault::getInstance()->setStringForKey(RECENT_FANS, Common::format("%d %d %d %d %d %d %d %d",
        static_cast<int>(recentFans[0]), static_cast<int>(recentFans[1]), static_cast<int>(recentFans[2]), static_cast<int>(recentFans[3]),
        static_cast<int>(recentFans[4]), static_cast<int>(recentFans[5]), static_cast<int>(recentFans[6]), static_cast<int>(recentFans[7])));
}

static FORCE_INLINE unsigned computeRowsAlign4(unsigned cnt) {
    return (cnt >> 2) + ((cnt & 0x3) != 0);
}

#define ORDER(flag_, i_) (((flag_) >> ((i_) << 1)) & 0x3U)

#define PLAYER_TO_UI(p_) ORDER(_seatFlag, (p_))
#define UI_TO_PLAYER(u_) ORDER(_playerFlag, (u_))

bool RecordScene::init(unsigned handIdx, const char **names, uint8_t mode, const Record::Detail *detail, SubmitCallback &&callback) {
    if (UNLIKELY(!BaseScene::initWithTitle(handNameText[handIdx]))) {
        return false;
    }

    static bool recentFansLoaded = false;
    if (!recentFansLoaded) {
        loadRecentFans();
        recentFansLoaded = true;
    }

    _handIdx = handIdx;
    memcpy(_playerNames, names, sizeof(_playerNames));
    _mode = mode;
    _submitCallback.swap(callback);

    bool isRealSeatOrder = !UserDefault::getInstance()->getBoolForKey(USE_FIXED_SEAT_ORDER);
    if (isRealSeatOrder) {
        switch (handIdx >> 2) {
        default: _seatFlag = 0xE4; _playerFlag = 0xE4; break;  // 3210 3210
        case 1: _seatFlag = 0xB1; _playerFlag = 0xB1; break;  // 2301 2301
        case 2: _seatFlag = 0x1E; _playerFlag = 0x4B; break;  // 0132 1023
        case 3: _seatFlag = 0x4B; _playerFlag = 0x1E; break;  // 1023 0132
        }
    }
    else {
        _seatFlag = 0xE4;
        _playerFlag = 0xE4;
    }

    _winIndex = -1;
    if (detail != nullptr) {
        memcpy(&_detail, detail, sizeof(_detail));
    }
    else {
        memset(&_detail, 0, sizeof(_detail));
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 帮助按钮
    ui::Button *button = ui::Button::create("icon/question-circle.png");
    this->addChild(button);
    button->setScale(24.0f / button->getContentSize().width);
    button->setPosition(Vec2(origin.x + visibleSize.width - 15.0f, origin.y + visibleSize.height - 15.0f));
    button->addClickEventListener(std::bind(&RecordScene::onInstructionButton, this, std::placeholders::_1));

    const float yPos = origin.y + visibleSize.height - 45.0f;
    // 番数输入框
    ui::EditBox *editBox = UICommon::createEditBox(Size(35.0f, 20.0f));
    this->addChild(editBox);
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::NUMERIC);
    editBox->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
    editBox->setFontColor(C4B_BLACK);
    editBox->setFontSize(12);
    editBox->setText("8");
    editBox->setMaxLength(3);  // 理论最高番332番，所以最大为3位
    editBox->setPosition(Vec2(origin.x + 84.0f, yPos));
    editBox->setDelegate(this);
    _editBox = editBox;

    Label *label = Label::createWithSystemFont(__UTF8("番"), "Arial", 12);
    label->setTextColor(C4B_BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 104.0f, yPos));

    // +-按钮
    static const float xPos[4] = { 18.0f, 48.0f, 133.0f, 163.0f };
    static const char *titleText[4] = { "-5", "-1", "+1", "+5" };
    static const int delta[4] = { -5, -1, 1, 5 };
    for (int i = 0; i < 4; ++i) {
        button = UICommon::createButton();
        this->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(25.0f, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText(titleText[i]);
        button->setPosition(Vec2(origin.x + xPos[i], yPos));
        button->setTag(delta[i]);
        button->addClickEventListener(std::bind(&RecordScene::onPlusButton, this, std::placeholders::_1));
    }

    // 荒庄
    ui::CheckBox *checkBox = UICommon::createCheckBox();
    this->addChild(checkBox);
    checkBox->setZoomScale(0.0f);
    checkBox->ignoreContentAdaptWithSize(false);
    checkBox->setContentSize(Size(20.0f, 20.0f));
    checkBox->setPosition(Vec2(origin.x + visibleSize.width - 50.0f, yPos));
    checkBox->addEventListener(std::bind(&RecordScene::onDrawTimeoutBox, this, std::placeholders::_1, std::placeholders::_2));
    _drawBox = checkBox;

    label = Label::createWithSystemFont(__UTF8("荒庄"), "Arial", 12);
    label->setTextColor(C4B_BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + visibleSize.width - 35.0f, yPos));

    // 超时
    checkBox = UICommon::createCheckBox();
    this->addChild(checkBox);
    checkBox->setZoomScale(0.0f);
    checkBox->ignoreContentAdaptWithSize(false);
    checkBox->setContentSize(Size(20.0f, 20.0f));
    checkBox->setPosition(Vec2(origin.x + visibleSize.width - 50.0f, yPos - 25.0f));
    checkBox->addEventListener(std::bind(&RecordScene::onDrawTimeoutBox, this, std::placeholders::_1, std::placeholders::_2));
    _timeoutBox = checkBox;

    label = Label::createWithSystemFont(__UTF8("超时"), "Arial", 12);
    label->setTextColor(C4B_BLACK);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + visibleSize.width - 35.0f, yPos - 25.0f));

    // 说明文本
    label = Label::createWithSystemFont(isRealSeatOrder ? __UTF8("当前模式为「换位」，选手顺序与当前圈座位相同") : __UTF8("当前模式为「固定」，选手顺序与开局座位相同"),
        "Arial", 10);
    label->setTextColor(C4B_GRAY);
    this->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(origin.x + 5.0f, yPos - 25.0f));
    cw::scaleLabelToFitWidth(label, visibleSize.width - 75.0f);

    ui::RadioButtonGroup *winGroup = ui::RadioButtonGroup::create();
    winGroup->setAllowedNoSelection(true);
    this->addChild(winGroup);
    winGroup->addEventListener(std::bind(&RecordScene::onWinGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    ui::RadioButtonGroup *claimGroup = ui::RadioButtonGroup::create();
    claimGroup->setAllowedNoSelection(true);
    this->addChild(claimGroup);
    claimGroup->addEventListener(std::bind(&RecordScene::onClaimGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    Node *radioNode = Node::create();
    this->addChild(radioNode);

    const float gap = (visibleSize.width - 4.0f) * 0.25f;
    for (int i = 0; i < 4; ++i) {
        const float x = origin.x + gap * (i + 0.5f);

        // 名字
        label = Label::createWithSystemFont(names[PLAYER_TO_UI(i)], "Arial", 12.0f);
        label->setTextColor(C4B_ORANGE);
        this->addChild(label);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 95.0f));
        cw::scaleLabelToFitWidth(label, gap - 4.0f);

        // 得分
        label = Label::createWithSystemFont("+0", "Arial", 12);
        label->setTextColor(C4B_GRAY);
        this->addChild(label);
        label->setPosition(Vec2(x, origin.y + visibleSize.height - 115.0f));
        _scoreLabel[i] = label;

        // 和牌
        float y = origin.y + visibleSize.height - 140.0f;
        ui::RadioButton *radioButton = UICommon::createRadioButton();
        radioNode->addChild(radioButton);
        radioButton->setZoomScale(0.0f);
        radioButton->ignoreContentAdaptWithSize(false);
        radioButton->setContentSize(Size(20.0f, 20.0f));
        radioButton->setPosition(Vec2(x - 15.0f, y));
        winGroup->addRadioButton(radioButton);

        label = Label::createWithSystemFont(__UTF8("和牌"), "Arial", 12);
        label->setTextColor(C4B_BLACK);
        radioNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, y));

        // 点炮或自摸
        y = origin.y + visibleSize.height - 170.0f;
        radioButton = UICommon::createRadioButton();
        radioNode->addChild(radioButton);
        radioButton->setZoomScale(0.0f);
        radioButton->ignoreContentAdaptWithSize(false);
        radioButton->setContentSize(Size(20.0f, 20.0f));
        radioButton->setPosition(Vec2(x - 15.0f, y));
        claimGroup->addRadioButton(radioButton);

        label = Label::createWithSystemFont(__UTF8("点炮"), "Arial", 12);
        label->setTextColor(C4B_BLACK);
        radioNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, y));
        _byDiscardLabel[i] = label;

        label = Label::createWithSystemFont(__UTF8("自摸"), "Arial", 12);
        label->setTextColor(C4B_BLACK);
        radioNode->addChild(label);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        label->setPosition(Vec2(x, y));
        label->setVisible(false);
        _selfDrawnLabel[i] = label;

        // 罚分
        y = origin.y + visibleSize.height - 195.0f;

        button = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_selected.png");
        button->setScale9Enabled(true);
        button->setContentSize(Size(gap - 8.0f, 20.0f));
        button->addClickEventListener(std::bind(&RecordScene::onPenaltyButton, this, std::placeholders::_1));
        radioNode->addChild(button);
        button->setPosition(Vec2(x, y));

        ui::RichText *richText = ui::RichText::create();
        radioNode->addChild(richText);
        richText->setPosition(Vec2(x, y));

        richText->pushBackElement(ui::RichElementText::create(0, Color3B(C4B_BLACK), 255, __UTF8("调整 "), "Arial", 12));
        richText->pushBackElement(ui::RichElementText::create(1, C3B_GRAY, 255, "+0", "Arial", 12));
        _penaltyText[i] = richText;
    }
    _winGroup = winGroup;
    _claimGroup = claimGroup;

    // 根结点
    ui::Layout *rootLayout = ui::Layout::create();
    this->addChild(rootLayout);
    rootLayout->setPosition(Vec2(origin.x, origin.y + 30.0f));
    rootLayout->setTouchEnabled(true);

    // 上方所有东西
    DrawNode *topNode = DrawNode::create();
    topNode->setContentSize(Size(visibleSize.width, 50.0f));
    topNode->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    topNode->setIgnoreAnchorPointForPosition(false);
    rootLayout->addChild(topNode);
    topNode->drawLine(Vec2(0.0f, 50.0f), Vec2(visibleSize.width, 50.0f), Color4F::BLACK);

    // 说明
    label = Label::createWithSystemFont(__UTF8("标记主番（4番以上）"), "Arial", 12);
    label->setTextColor(C4B_BLACK);
    topNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5.0f, 35.0f));

    // 展开/收起
    ui::Button *layoutButton = UICommon::createButton();
    layoutButton->setScale9Enabled(true);
    layoutButton->setContentSize(Size(45.0f, 20.0f));
    layoutButton->setTitleFontSize(12);
    topNode->addChild(layoutButton);
    layoutButton->setPosition(Vec2(visibleSize.width - 30.0f, 35.0f));

    button = UICommon::createButton();
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText(__UTF8("记录和牌"));
    button->addClickEventListener([this](Ref *) { onRecordTilesButton(nullptr); });
    topNode->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 85.0f, 35.0f));
    _recordTilesButton = button;

    // 小番
    button = UICommon::createButton();
    button->setScale9Enabled(true);
    button->setContentSize(Size(45.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText(__UTF8("小番"));
    button->addClickEventListener([this](Ref *) { showLittleFanAlert(false); });
    topNode->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 30.0f, 10.0f));
    _littleFanButton = button;

    cw::TableView *tableView = cw::TableView::create();
    tableView->setDirection(ui::ScrollView::Direction::VERTICAL);
    tableView->setScrollBarPositionFromCorner(Vec2(2.0f, 2.0f));
    tableView->setScrollBarWidth(4.0f);
    tableView->setScrollBarOpacity(0x99);
    tableView->setDelegate(this);
    tableView->setVerticalFillOrder(cw::TableView::VerticalFillOrder::TOP_DOWN);

    tableView->setAnchorPoint(Vec2::ANCHOR_MIDDLE_BOTTOM);
    tableView->setPosition(Vec2(visibleSize.width * 0.5f, 0.0f));
    rootLayout->addChild(tableView);
    _tableView = tableView;

    std::function<void (Ref *)> onLayoutButton = [radioNode, rootLayout, topNode, tableView](Ref *sender) {
        ui::Button *layoutButton = (ui::Button *)sender;

        static const std::string layoutText[2] = {
            std::string("\xE2\xAC\x87\xEF\xB8\x8E").append(__UTF8("收起")),
            std::string("\xE2\xAC\x86\xEF\xB8\x8E").append(__UTF8("展开"))
        };
        Size visibleSize = Director::getInstance()->getVisibleSize();
        Size layoutSize;
        layoutSize.width = visibleSize.width;
        if (layoutButton->getUserData()) {
            layoutSize.height = visibleSize.height - 155.0f;
            layoutButton->setUserData(reinterpret_cast<void *>(false));
            layoutButton->setTitleText(layoutText[0]);
            radioNode->setVisible(false);
        }
        else {
            layoutSize.height = visibleSize.height - 240.0f;
            layoutButton->setUserData(reinterpret_cast<void *>(true));
            layoutButton->setTitleText(layoutText[1]);
            radioNode->setVisible(true);
        }

        rootLayout->setContentSize(layoutSize);
        topNode->setPosition(Vec2(visibleSize.width * 0.5f, layoutSize.height - 25.0f));
        tableView->setContentSize(Size(visibleSize.width - 5.0f, layoutSize.height - 55.0f));
        tableView->reloadData();
    };
    onLayoutButton(layoutButton);
    layoutButton->addClickEventListener(onLayoutButton);

    // 跳到
    label = Label::createWithSystemFont(__UTF8("跳到"), "Arial", 12);
    label->setTextColor(C4B_BLACK);
    topNode->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(5.0f, 10.0f));

    // 6 8 12 16 24
    const float labelPosX = label->getContentSize().width + 5.0f + 2.0f;
    static const char *text[] = { __UTF8("6番"), __UTF8("8番"), __UTF8("12番"), __UTF8("16番"), __UTF8("24番") };
    for (unsigned i = 0; i < 5; ++i) {
        button = UICommon::createButton();
        button->setScale9Enabled(true);
        button->setContentSize(Size(30.0f, 20.0f));
        button->setTitleFontSize(12);
        button->setTitleText(text[i]);
        button->addClickEventListener([tableView, i](Ref *) { tableView->jumpToCell(i + 2); });
        topNode->addChild(button);
        button->setPosition(Vec2(labelPosX + 35.0f * (0.5f + i), 10.0f));
    }

    // 提交按钮
    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(50.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText(__UTF8("提交"));
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + 15.0f));
    button->addClickEventListener(std::bind(&RecordScene::onSubmitButton, this, std::placeholders::_1));
    button->setEnabled(false);
    _submitButton = button;

    if (detail != nullptr) {
        refresh();
    }
    return true;
}

ssize_t RecordScene::numberOfCellsInTableView(cw::TableView *) {
    return 11;
}

float RecordScene::tableCellSizeForIndex(cw::TableView *, ssize_t idx) {
    unsigned cnt = cellDetails[idx].count;
    float height = computeRowsAlign4(cnt) * 25.0f;
    return (height + 15.0f);
}

namespace {
    typedef cw::TableViewCellEx<Label *, std::array<ui::CheckBox *, 9>, std::array<Label *, 9> > CustomCell;
}

cw::TableViewCell *RecordScene::tableCellAtIndex(cw::TableView *table, ssize_t idx) {
    CustomCell *cell = (CustomCell *)table->dequeueCell();

    const float cellWidth = table->getContentSize().width;
    const float gap = cellWidth * 0.25f;

    if (cell == nullptr) {
        cell = CustomCell::create();

        CustomCell::ExtDataType &ext = cell->getExtData();
        Label *&label = std::get<0>(ext);
        ui::CheckBox **checkBoxes = std::get<1>(ext).data();
        Label **titleLabels = std::get<2>(ext).data();

        label = Label::createWithSystemFont(__UTF8("1番"), "Arial", 12);
        label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        cell->addChild(label);
        label->setTextColor(C4B_BLACK);

        for (unsigned k = 0; k < 9; ++k) {
            Size size(gap - 4.0f, 20.0f);
            Vec2 pos(size.width * 0.5f, size.height * 0.5f);

            CheckBoxScale9 *checkBox = CheckBoxScale9::create();
            checkBox->loadTextures("source_material/btn_square_normal.png",
                "source_material/btn_square_selected.png", "source_material/btn_square_highlighted.png",
                "source_material/btn_radio_disabled.png", "source_material/btn_radio_disabled.png");
            checkBox->setZoomScale(0.0f);
            checkBox->ignoreContentAdaptWithSize(false);
            checkBox->setContentSize(size);
            checkBox->addEventListener(std::bind(&RecordScene::onFanNameBox, this, std::placeholders::_1, std::placeholders::_2));
            cell->addChild(checkBox);
            checkBoxes[k] = checkBox;

            Label *titleLabel = Label::createWithSystemFont("", "Arail", 12);
            titleLabel->setTextColor(C4B_GRAY);
            checkBox->addChild(titleLabel);
            titleLabel->setPosition(pos);
            titleLabels[k] = titleLabel;
        }
    }

    const CellDetail &detail = cellDetails[idx];
    const unsigned currentLevelCount = detail.count;
    unsigned totalRows = computeRowsAlign4(currentLevelCount);

    CustomCell::ExtDataType &ext = cell->getExtData();
    Label *label = std::get<0>(ext);
    ui::CheckBox *const *checkBoxes = std::get<1>(ext).data();
    Label *const *titleLabels = std::get<2>(ext).data();

    label->setString(detail.title);
    label->setPosition(Vec2(5.0f, totalRows * 25.0f + 7.0f));

    const mahjong::fan_t *fans = detail.fans;
    for (unsigned k = 0; k < currentLevelCount; ++k) {
        mahjong::fan_t fan = fans[k];

        ui::CheckBox *checkBox = checkBoxes[k];
        Label *titleLabel = titleLabels[k];
        checkBox->setUserData(reinterpret_cast<void *>(idx));
        checkBox->setTag(static_cast<int>(fan));

        checkBox->setVisible(true);
        titleLabel->setString(mahjong::fan_name[fan]);
        cw::scaleLabelToFitWidth(titleLabel, gap - 8.0f);

        unsigned col = k & 0x3;
        unsigned row = k >> 2;
        checkBox->setPosition(Vec2(gap * (col + 0.5f), (totalRows - row - 0.5f) * 25.0f));
        checkBox->setSelected(TEST_FAN(_detail.fan_bits, fan));
    }

    for (unsigned k = currentLevelCount; k < 9; ++k) {
        ui::CheckBox *checkBox = checkBoxes[k];
        checkBox->setVisible(false);
        checkBox->setSelected(false);
    }

    return cell;
}

void RecordScene::editBoxEditingDidBegin(cocos2d::ui::EditBox *editBox) {
    // 点击后清除内容的效果
    editBox->setPlaceHolder(editBox->getText());
    editBox->setText("");
}

void RecordScene::editBoxReturn(cocos2d::ui::EditBox *editBox) {
    const char *text = editBox->getText();
    if (*text == '\0') {
        const char *placeholder = editBox->getPlaceHolder();
        editBox->setText(placeholder);
        text = placeholder;
    }

    if (atoi(text) < 8) {
        editBox->setText("8");
    }

    updateScoreLabel();
}

static inline void updatePenaltyLabel(Label *label, int16_t ps) {
    label->setString(Common::format("%+hd", ps));

    if (ps < 0) label->setTextColor(C4B_PURPLE);
    else if (ps > 0) label->setTextColor(C4B_RED);
    else label->setTextColor(C4B_GRAY);
}

static inline void updatePenaltyText(ui::RichText *richText, int16_t ps) {
    Color3B color;
    if (ps < 0) color = Color3B(C4B_PURPLE);
    else if (ps > 0) color = Color3B(C4B_RED);
    else color = Color3B(C4B_GRAY);

    richText->removeElement(1);
    richText->pushBackElement(ui::RichElementText::create(0, color, 255, Common::format("%+hd", ps), "Arial", 12));
}

void RecordScene::refresh() {
    uint8_t wf = _detail.win_flag;
    uint8_t cf = _detail.claim_flag;
    uint16_t fan = _detail.fan;
    if (fan >= 8) {
        char str[32];
        snprintf(str, sizeof(str), "%hu", fan);
        _editBox->setText(str);
    }

    // 罚分
    for (int i = 0; i < 4; ++i) {
        updatePenaltyText(_penaltyText[i], _detail.penalty_scores[PLAYER_TO_UI(i)]);
    }

    _winIndex = WIN_CLAIM_INDEX(wf);
    if (_winIndex != -1) {  // 有人和牌
        int claimIndex = WIN_CLAIM_INDEX(cf);  // 点炮者
        _winGroup->setSelectedButton(UI_TO_PLAYER(_winIndex));
        if (claimIndex != -1) {
            _claimGroup->setSelectedButton(UI_TO_PLAYER(claimIndex));
        }
    }

    if (fan == 0) {
        if (UNLIKELY(_detail.timeout)) {
            _timeoutBox->setSelected(true);
            onDrawTimeoutBox(_timeoutBox, ui::CheckBox::EventType::SELECTED);
        } else {
            _drawBox->setSelected(true);
            onDrawTimeoutBox(_drawBox, ui::CheckBox::EventType::SELECTED);
        }
    }
    else {
        updateScoreLabel();
    }

    _tableView->reloadDataInplacement();
}

void RecordScene::SetScoreLabelColor(cocos2d::Label *(&scoreLabel)[4], int (&scoreTable)[4], uint8_t win_flag, uint8_t claim_flag, const int16_t (&penalty_scores)[4]) {
    for (int i = 0; i < 4; ++i) {
        if (scoreTable[i] != 0) {
            if (TEST_WIN_CLAIM(win_flag, i)) {  // 和牌：红色
                scoreLabel[i]->setTextColor(C4B_RED);
            }
            else {
                if (UNLIKELY(penalty_scores[i] < 0)) {  // 罚分：紫色
                    scoreLabel[i]->setTextColor(C4B_PURPLE);
                }
                else if (UNLIKELY(TEST_WIN_CLAIM(claim_flag, i))) {  // 点炮：蓝色
                    scoreLabel[i]->setTextColor(C4B_BLUE);
                }
                else {  // 其他：绿色
                    scoreLabel[i]->setTextColor(C4B_GREEN);
                }
            }
        }
        else {
            scoreLabel[i]->setTextColor(C4B_GRAY);
        }
    }
}

void RecordScene::updateScoreLabel() {
    _detail.win_flag = 0;
    _detail.claim_flag = 0;
    int claimIndex = -1;
    if (_winIndex != -1) {  // 有人和牌
        int fan = atoi(_editBox->getText());  // 获取输入框里所填番数
        _detail.fan = static_cast<uint16_t>(std::max(8, fan));
        claimIndex = _claimGroup->getSelectedButtonIndex();

        // 记录和牌和点炮
        SET_WIN_CLAIM(_detail.win_flag, PLAYER_TO_UI(_winIndex));
        if (claimIndex != -1) {
            SET_WIN_CLAIM(_detail.claim_flag, PLAYER_TO_UI(claimIndex));
        }
    }
    else {  // 荒庄或者超时
        _detail.fan = 0;
        _detail.timeout = _timeoutBox->isSelected();
    }

    int scoreTable[4];
    TranslateDetailToScoreTable(_detail, _mode, scoreTable);

    for (int i = 0; i < 4; ++i) {
        _scoreLabel[i]->setString(Common::format("%+d", scoreTable[PLAYER_TO_UI(i)]));
    }

    // 使用不同颜色
    Label *tempLabel[4] = { _scoreLabel[UI_TO_PLAYER(0)], _scoreLabel[UI_TO_PLAYER(1)], _scoreLabel[UI_TO_PLAYER(2)], _scoreLabel[UI_TO_PLAYER(3)] };
    SetScoreLabelColor(tempLabel, scoreTable, _detail.win_flag, _detail.claim_flag, _detail.penalty_scores);

    // 未选择和牌
    if (_winIndex == -1) {
        // 荒庄或者超时时允许确定
        _submitButton->setEnabled(_drawBox->isSelected() || _timeoutBox->isSelected());
    }
    else {
        // 未选择是点炮还是自摸时，不允许确定
        _submitButton->setEnabled(claimIndex != -1);
    }
}

void RecordScene::onInstructionButton(cocos2d::Ref *) {
    const float width = AlertDialog::maxWidth();
    Label *label = Label::createWithSystemFont(
        __UTF8("1. 「换位」模式下，选手顺序与当前圈座位相同；「固定」模式下，选手顺序与开局座位相同。可通过上一级界面的「更多设置」进行切换。\n")
        __UTF8("2. 番数框支持直接输入，「标记主番」可快速增加番数，强烈建议先「标记主番」，再用两侧的+和-按钮调整。\n")
        __UTF8("3. 「荒庄」与「超时」的区别是，「超时」的盘数在汇总时不计入有效盘数。\n")
        __UTF8("4. 「标记主番」出于简化代码逻辑考虑，未做排斥检测，即程序允许标记若干个自相矛盾的主番，但不建议你这样做。\n")
        __UTF8("5. 「小番」为2番及1番的番种。\n")
        __UTF8("6. 「记录和牌」可根据当前和牌自动算番，自动标记番种。")
        , "Arial", 10, Size(width, 0.0f));
    label->setTextColor(C4B_BLACK);
    label->setLineSpacing(2.0f);

    AlertDialog::Builder(Director::getInstance()->getRunningScene())
        .setTitle(__UTF8("使用说明"))
        .setContentNode(label)
        .setCloseOnTouchOutside(false)
        .setPositiveButton(__UTF8("确定"), nullptr)
        .create()->show();
}

void RecordScene::onPlusButton(cocos2d::Ref *sender) {
    int delta = ((ui::Button *)sender)->getTag();
    int winScore = atoi(_editBox->getText());
    int temp = winScore + delta;
    if (temp < 8) temp = 8;
    if (winScore != temp) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", temp);
        _editBox->setText(buf);
        updateScoreLabel();
    }
}

void RecordScene::onRecordTilesButton(cocos2d::Ref *) {
    if (_drawBox->isSelected()) {
        Toast::makeText(this, __UTF8("荒庄时不能记录和牌"), Toast::LENGTH_LONG)->show();
        return;
    }

    mahjong::calculate_param_t param;

    mahjong::string_to_tiles(_detail.win_hand.tiles, &param.hand_tiles, &param.win_tile);
    param.win_flag = _detail.win_hand.win_flag;
    param.flower_count = _detail.win_hand.flower_count;

    int idx = _winGroup->getSelectedButtonIndex();
    if (idx >= 0 && idx < 4) {
        param.seat_wind = static_cast<mahjong::wind_t>((idx + 4 - (_handIdx & 0x3)) & 0x3);
    }

    showCalculator(param);
}

void RecordScene::onDrawTimeoutBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    if (event == ui::CheckBox::EventType::SELECTED) {
        _tableView->setEnabled(false);
        _recordTilesButton->setEnabled(false);
        _littleFanButton->setEnabled(false);
        if (sender == _drawBox) {
            _timeoutBox->setEnabled(false);
        }
        else {
            _drawBox->setEnabled(false);
        }

        _winIndex = -1;
        // 禁用所有人的和、自摸/点炮、第二行显示点炮
        for (int i = 0; i < 4; ++i) {
            _winGroup->getRadioButtonByIndex(i)->setEnabled(false);
            _claimGroup->getRadioButtonByIndex(i)->setEnabled(false);
            _byDiscardLabel[i]->setVisible(true);
            _selfDrawnLabel[i]->setVisible(false);
        }
    }
    else {
        _tableView->setEnabled(true);
        _recordTilesButton->setEnabled(true);
        _littleFanButton->setEnabled(true);
        if (sender == _drawBox) {
            _timeoutBox->setEnabled(true);
        }
        else {
            _drawBox->setEnabled(true);
        }

        _winIndex = _winGroup->getSelectedButtonIndex();
        // 启用所有人的和、自摸/点炮
        for (int i = 0; i < 4; ++i) {
            _winGroup->getRadioButtonByIndex(i)->setEnabled(true);
            _claimGroup->getRadioButtonByIndex(i)->setEnabled(true);
        }
        // 和的选手，显示自摸
        if (_winIndex != -1) {
            _byDiscardLabel[_winIndex]->setVisible(false);
            _selfDrawnLabel[_winIndex]->setVisible(true);
        }
    }
    updateScoreLabel();
}

void RecordScene::onWinGroup(cocos2d::ui::RadioButton *, int index, cocos2d::ui::RadioButtonGroup::EventType) {
    int prevIndex = _winIndex;
    _winIndex = index;

    if (prevIndex != -1) {
        // 未和的选手，显示点炮
        _byDiscardLabel[prevIndex]->setVisible(true);
        _selfDrawnLabel[prevIndex]->setVisible(false);
    }
    // 和的选手，显示自摸
    _byDiscardLabel[index]->setVisible(false);
    _selfDrawnLabel[index]->setVisible(true);

    updateScoreLabel();
}

void RecordScene::onClaimGroup(cocos2d::ui::RadioButton *, int, cocos2d::ui::RadioButtonGroup::EventType) {
    if (_winIndex == -1) return;
    updateScoreLabel();
}

void RecordScene::onPenaltyButton(cocos2d::Ref *) {
    float maxWidth = AlertDialog::maxWidth();

    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(maxWidth, 170.0f));

    const float gap = (maxWidth - 4.0f) * 0.25f;

    static const int16_t value[4] = { -10, -5, +5, +10 };
    static const char *text[4] = { "-10", "-5", "+5", "+10" };
    static const float buttonY[4] = { 135.0f, 110.0f, 60.0f, 35.0f };

    std::shared_ptr<std::array<int16_t, 4> > penaltyScoresStrong = std::make_shared<std::array<int16_t, 4> >();
    int16_t *penaltyScores = penaltyScoresStrong->data();
    memcpy(penaltyScores, &_detail.penalty_scores, sizeof(_detail.penalty_scores));

    int16_t sum = penaltyScores[0] + penaltyScores[1] + penaltyScores[2] + penaltyScores[3];
    Label *label = Label::createWithSystemFont(Common::format(__UTF8("校验：%hd"), sum), "Arial", 12.0f);
    label->setTextColor(sum == 0 ? C4B_GRAY : C4B_RED);
    rootNode->addChild(label);
    label->setPosition(Vec2(maxWidth * 0.5f, 10.0f));
    Label *checkLabel = label;

    for (int i = 0; i < 4; ++i) {
        const float x = gap * (i + 0.5f);

        // 名字
        label = Label::createWithSystemFont(_playerNames[PLAYER_TO_UI(i)], "Arial", 12.0f);
        label->setTextColor(C4B_ORANGE);
        rootNode->addChild(label);
        label->setPosition(Vec2(x, 160.0f));
        cw::scaleLabelToFitWidth(label, gap - 2.0f);

        ui::Scale9Sprite *sprite = ui::Scale9Sprite::create("source_material/btn_square_normal.png");
        rootNode->addChild(sprite);
        sprite->setContentSize(Size(30.0f, 20.0f));
        sprite->setPosition(Vec2(x, 85.0f));

        // 罚分
        label = Label::createWithSystemFont("", "Arial", 12);
        rootNode->addChild(label);
        label->setPosition(Vec2(x, 85.0f));
        updatePenaltyLabel(label, penaltyScores[PLAYER_TO_UI(i)]);

        for (int n = 0; n < 4; ++n) {
            ui::Button *button = UICommon::createButton();
            button->setScale9Enabled(true);
            button->setContentSize(Size(30.0f, 20.0f));
            button->setTitleFontSize(12);
            button->setTitleText(text[n]);
            rootNode->addChild(button);
            button->setPosition(Vec2(x, buttonY[n]));
            int v = value[n];
            button->addClickEventListener([this, penaltyScores, label, checkLabel, i, v](Ref *) {
                int16_t &ps = penaltyScores[PLAYER_TO_UI(i)];
                ps += v;
                updatePenaltyLabel(label, ps);

                int16_t sum = penaltyScores[0] + penaltyScores[1] + penaltyScores[2] + penaltyScores[3];
                checkLabel->setString(Common::format(__UTF8("校验：%hd"), sum));
                checkLabel->setTextColor(sum == 0 ? C4B_GRAY : C4B_RED);
            });
        }
    }

    AlertDialog::Builder(this)
        .setTitle(__UTF8("罚分调整"))
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setNegativeButton(__UTF8("取消"), nullptr)
        .setPositiveButton(__UTF8("确定"), [this, penaltyScoresStrong](AlertDialog *, int) {
        memcpy(&_detail.penalty_scores, penaltyScoresStrong->data(), sizeof(_detail.penalty_scores));
        for (int i = 0; i < 4; ++i) {
            updatePenaltyText(_penaltyText[i], _detail.penalty_scores[PLAYER_TO_UI(i)]);
        }
        updateScoreLabel();
        return true;
    }).create()->show();
}

static void refreshLittleFanButton(ui::Button *button, unsigned cnt, int idx) {
    if (cnt == 0) {
        if (button->getNormalFile().file.compare("source_material/btn_square_normal.png") != 0) {
            button->loadTextureNormal("source_material/btn_square_normal.png");
        }
    }
    else {
        if (button->getNormalFile().file.compare("source_material/btn_square_highlighted.png") != 0) {
            button->loadTextureNormal("source_material/btn_square_highlighted.png");
        }
    }
    if (cnt < 2) {
        button->setTitleText(mahjong::fan_name[idx]);
    }
    else {
        button->setTitleText(Common::format("%s\xC3\x97%u", mahjong::fan_name[idx], cnt));
    }
    cw::scaleLabelToFitWidth(button->getTitleLabel(), button->getContentSize().width - 4.0f);
};

void RecordScene::showLittleFanAlert(bool callFromSubmitting) {
    uint32_t fan2Bits = _detail.fan2_bits;
    uint64_t fan1Bits = _detail.fan1_bits;

    const float maxWidth = AlertDialog::maxWidth();
    const float maxHeight = Director::getInstance()->getVisibleSize().height * 0.8f - 80.0f;

    Node *rootNode = Node::create();
    ui::Widget *containerWidget = ui::Widget::create();

    static const uint8_t limitCounts[] = { 1, 1, 1, 1, 1, 3, 2, 1, 1, 1, 2, 2, 2, 2, 4, 1, 3, 1, 1, 1, 1, 1, 8, 1 };

    const float width4 = maxWidth * 0.25f;
    auto bitsData = std::make_shared<std::pair<uint32_t, uint64_t> >(std::make_pair(fan2Bits, fan1Bits));
    std::pair<uint32_t, uint64_t> *bits = bitsData.get();

    // 2番点击回调
    std::function<void (Ref *)> on2FanButtonClick = [bits](Ref *sender) {
        ui::Button *button = (ui::Button *)sender;
        int idx = button->getTag();

        uint8_t prevCnt = COUNT_FAN2(bits->first, idx);
        uint8_t tempCnt = prevCnt + 1;
        if (tempCnt > limitCounts[idx]) {
            tempCnt = 0;
        }

        RESET_FAN2(bits->first, idx);
        SET_FAN2(bits->first, idx, tempCnt);
        refreshLittleFanButton(button, tempCnt, mahjong::DRAGON_PUNG + idx);
    };

    // 1番点击回调
    std::function<void (Ref *)> on1FanButtonClick = [bits](Ref *sender) {
        ui::Button *button = (ui::Button *)sender;
        int idx = button->getTag();

        uint8_t prevCnt = COUNT_FAN1(bits->second, idx);
        uint8_t tempCnt = prevCnt + 1;
        if (tempCnt > limitCounts[10 + idx]) {
            tempCnt = 0;
        }
        RESET_FAN1(bits->second, idx);
        SET_FAN1(bits->second, idx, tempCnt);
        refreshLittleFanButton(button, tempCnt, mahjong::PURE_DOUBLE_CHOW + idx);
    };

#if SUPPORT_CONCEALED_KONG_AND_MELDED_KONG
    const float containerHeight = 255.0f;
#else
    const float containerHeight = 210.0f;
#endif

    // 2番
    Label *label = Label::createWithSystemFont(__UTF8("2番"), "Arial", 12);
    label->setTextColor(C4B_BLACK);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    containerWidget->addChild(label);
    label->setPosition(Vec2(5.0f, containerHeight - 10.0f));

    for (int i = 0; i < 10; ++i) {
        div_t ret = div(i, 4);
        const float yPos = containerHeight - 30.0f - ret.quot * 25.0f;
        const float xPos = width4 * (ret.rem + 0.5f);

        ui::Button *button = ui::Button::create();
        button->loadTexturePressed("source_material/btn_square_selected.png");
        button->setScale9Enabled(true);
        button->setContentSize(Size(width4 - 5.0f, 20.0f));
        button->setTitleColor(C3B_GRAY);
        button->setTitleFontSize(12);
        button->setTag(i);
        button->addClickEventListener(on2FanButtonClick);
        containerWidget->addChild(button);
        button->setPosition(Vec2(xPos, yPos));

        uint32_t cnt = COUNT_FAN2(fan2Bits, i);
        refreshLittleFanButton(button, cnt, mahjong::DRAGON_PUNG + i);
    }

    // 1番
    label = Label::createWithSystemFont(__UTF8("1番"), "Arial", 12);
    label->setTextColor(C4B_BLACK);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    containerWidget->addChild(label);
    label->setPosition(Vec2(5.0f, containerHeight - 105.0f));

    for (int i = 0; i < 13; ++i) {
        div_t ret = div(i, 4);
        const float yPos = containerHeight - 125.0f - ret.quot * 25.0f;
        const float xPos = width4 * (ret.rem + 0.5f);

        ui::Button *button = ui::Button::create();
        button->loadTexturePressed("source_material/btn_square_selected.png");
        button->setScale9Enabled(true);
        button->setContentSize(Size(width4 - 5.0f, 20.0f));
        button->setTitleColor(C3B_GRAY);
        button->setTitleFontSize(12);
        button->setTag(i);
        button->addClickEventListener(on1FanButtonClick);
        containerWidget->addChild(button);
        button->setPosition(Vec2(xPos, yPos));

        uint32_t cnt = COUNT_FAN1(fan1Bits, i);
        refreshLittleFanButton(button, cnt, mahjong::PURE_DOUBLE_CHOW + i);
    }

#if SUPPORT_CONCEALED_KONG_AND_MELDED_KONG
    label = Label::createWithSystemFont(__UTF8("5番"), "Arial", 12);
    label->setTextColor(C4B_BLACK);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    containerWidget->addChild(label);
    label->setPosition(Vec2(5.0f, 30.0f));

    {
        const int i = mahjong::CONCEALED_KONG_AND_MELDED_KONG - mahjong::PURE_DOUBLE_CHOW;

        ui::Button *button = ui::Button::create();
        button->loadTexturePressed("source_material/btn_square_selected.png");
        button->setScale9Enabled(true);
        button->setContentSize(Size(width4 - 5.0f, 20.0f));
        button->setTitleColor(C3B_GRAY);
        button->setTitleFontSize(12);
        button->setTag(i);
        button->addClickEventListener(on1FanButtonClick);
        containerWidget->addChild(button);
        button->setPosition(Vec2(width4 * 0.5f, 10.0f));

        uint32_t cnt = COUNT_FAN1(fan1Bits, i);
        refreshLittleFanButton(button, cnt, mahjong::CONCEALED_KONG_AND_MELDED_KONG);
    }
#endif

    // 超出高度就使用ScrollView
    if (maxHeight >= containerHeight + 45.0f) {
        rootNode->setContentSize(Size(maxWidth, containerHeight + 45.0f));
        containerWidget->setContentSize(Size(maxWidth, containerHeight));
    }
    else {
        rootNode->setContentSize(Size(maxWidth, maxHeight));

        ui::ScrollView *scrollView = ui::ScrollView::create();
        scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
        scrollView->setScrollBarPositionFromCorner(Vec2(2.0f, 2.0f));
        scrollView->setScrollBarWidth(4.0f);
        scrollView->setScrollBarOpacity(0x99);
        scrollView->setContentSize(Size(maxWidth, maxHeight - 45.0f));
        scrollView->setInnerContainerSize(Size(maxWidth, containerHeight));
        scrollView->addChild(containerWidget);

        containerWidget = scrollView;
    }
    rootNode->addChild(containerWidget);
    containerWidget->setAnchorPoint(Vec2::ANCHOR_MIDDLE_BOTTOM);
    containerWidget->setPosition(Vec2(maxWidth * 0.5f, 45.0f));

    label = Label::createWithSystemFont(__UTF8("点击按钮可叠加，超过上限时回到0"), "Arial", 10);
    label->setTextColor(C4B_BLACK);
    rootNode->addChild(label);
    label->setPosition(Vec2(maxWidth * 0.5f, 30.0f));
    cw::scaleLabelToFitWidth(label, maxWidth);

    ui::CheckBox *checkBox = UICommon::createCheckBox();
    checkBox->setZoomScale(0.0f);
    checkBox->ignoreContentAdaptWithSize(false);
    checkBox->setContentSize(Size(15.0f, 15.0f));
    checkBox->setPosition(Vec2(9.5f, 10.0f));
    rootNode->addChild(checkBox);
    checkBox->setSelected(UserDefault::getInstance()->getBoolForKey(NEVER_NOTIFY_LITTLE_FAN, false));

    label = Label::createWithSystemFont(__UTF8("不再提示标记小番"), "Arial", 10);
    label->setTextColor(C4B_GRAY);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    rootNode->addChild(label);
    label->setPosition(Vec2(22.0f, 10.0f));

    AlertDialog::Builder(this)
        .setTitle(__UTF8("标记小番"))
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setNegativeButton(__UTF8("取消"), nullptr)
        .setPositiveButton(__UTF8("确定"), [this, bitsData, checkBox, callFromSubmitting](AlertDialog *, int) {
        UserDefault::getInstance()->setBoolForKey(NEVER_NOTIFY_LITTLE_FAN, checkBox->isSelected());

        _detail.fan1_bits = bitsData->second;
        _detail.fan2_bits = bitsData->first;
        if (callFromSubmitting) {
            finish();
        }
        return true;
    }).create()->show();
}

static const ssize_t standardFanToCellIdx[mahjong::FAN_TABLE_SIZE] {
    0,
    10, 10, 10, 10, 10, 10, 10,
    9, 9, 9, 9, 9, 9,
    8, 8,
    7, 7, 7,
    6, 6, 6, 6, 6, 6, 6, 6, 6,
    5, 5, 5, 5, 5, 5,
    4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1,
};

void RecordScene::onFanNameBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event) {
    ui::CheckBox *checkBox = (ui::CheckBox *)sender;
    ssize_t cellIdx = reinterpret_cast<ssize_t>(checkBox->getUserData());
    const mahjong::fan_t fan = static_cast<mahjong::fan_t>(checkBox->getTag());

    // 标记/取消标记番种
    if (event == ui::CheckBox::EventType::SELECTED) {
        SET_FAN(_detail.fan_bits, fan);
    }
    else {
        RESET_FAN(_detail.fan_bits, fan);
    }

    // 计算番数
    int prevWinScore = atoi(_editBox->getText());
    int currentWinScore = 0;
    for (int n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
        if (TEST_FAN(_detail.fan_bits, n)) {
            currentWinScore += mahjong::fan_value_table[n];
        }
    }
    currentWinScore = std::max(8, currentWinScore);

    // 增加番数
    if (currentWinScore > prevWinScore) {
        char str[16];
        snprintf(str, sizeof(str), "%d", currentWinScore);
        _editBox->setText(str);
    }
    updateScoreLabel();

    // 点击的不是「最近使用」
    if (cellIdx != 0) {
        // 如果该番在「最近使用」里，则更新
        const auto it = std::find(std::begin(recentFans), std::end(recentFans), fan);
        if (it != std::end(recentFans)) {
            CustomCell *cell = (CustomCell *)_tableView->cellAtIndex(0);
            if (cell != nullptr) {  // 「最近使用」的cell在绘制
                // 相应CheckBox的下标
                const ptrdiff_t idx = it - std::begin(recentFans);

                // 刷新CheckBox
                ui::CheckBox **checkBoxes = std::get<1>(cell->getExtData()).data();
                if (LIKELY(fan == cellDetails[0].fans[idx])) {
                    checkBoxes[idx]->setSelected(TEST_FAN(_detail.fan_bits, fan));
                }
                else {
                    UNREACHABLE();
                }
            }
        }
    }
    else {
        // 点击「最近使用」，更新找到对应cell的番，更新
        cellIdx = standardFanToCellIdx[fan];  // 对应cell下标
        if (cellIdx != 0) {
            CustomCell *cell = (CustomCell *)_tableView->cellAtIndex(cellIdx);
            if (cell != nullptr) {  // 该cell在绘制
                // 相应CheckBox的下标
                const CellDetail &detail = cellDetails[cellIdx];
                const ssize_t idx = static_cast<ssize_t>(fan) - static_cast<ssize_t>(detail.first_fan);

                // 刷新CheckBox
                ui::CheckBox **checkBoxes = std::get<1>(cell->getExtData()).data();
                if (LIKELY(fan == detail.fans[idx])) {
                    checkBoxes[idx]->setSelected(TEST_FAN(_detail.fan_bits, fan));
                }
                else {
                    UNREACHABLE();
                }
            }
        }
    }
}

void RecordScene::adjustRecentFans() {
    uint64_t fanBits = _detail.fan_bits;
    if (fanBits == 0) {
        return;
    }

    mahjong::fan_t temp[8];
    uint8_t cnt = 0;

    // 1. 将所有标记的番写到temp
    for (int fan = mahjong::BIG_FOUR_WINDS; fan < mahjong::DRAGON_PUNG; ++fan) {
        if (TEST_FAN(fanBits, fan)) {
            temp[cnt++] = static_cast<mahjong::fan_t>(fan);
            if (cnt >= 8) {
                break;
            }
        }
    }

    // 2. 对于不满8个的，补充原来recentFans的数据，注意去重
    if (cnt < 8) {
        for (int i = 0; i < 8; ++i) {
            mahjong::fan_t fan = recentFans[i];
            if (TEST_FAN(fanBits, fan)) {
                continue;
            }
            temp[cnt++] = fan;
            if (cnt >= 8) {
                break;
            }
        }
    }

    cellDetails[0].count = cnt;
    std::copy(std::begin(temp), std::begin(temp) + cnt, std::begin(recentFans));
    saveRecentFans();
}

void RecordScene::onSubmitButton(cocos2d::Ref *) {
    if (_detail.fan_bits != 0 || _detail.fan2_bits != 0 || _detail.fan1_bits != 0) {  // 标记了番种
        if (_drawBox->isSelected() || _timeoutBox->isSelected()) {  // 荒庄或者超时
            AlertDialog::Builder(this)
                .setTitle(__UTF8("计分"))
                .setMessage(__UTF8("选择「荒庄」或者「超时」将忽略标记的番种"))
                .setNegativeButton(__UTF8("取消"), nullptr)
                .setPositiveButton(__UTF8("确定"), [this](AlertDialog *, int) {
                _detail.fan_bits = 0;
                _detail.fan2_bits = 0;
                _detail.fan1_bits = 0;
                memset(&_detail.win_hand, 0, sizeof(_detail.win_hand));
                finish();
                return true;
            }).create()->show();
        }
        else {
            finish();
        }
    }
    else {  // 未标记番种
        bool shouldShow = !UserDefault::getInstance()->getBoolForKey(NEVER_NOTIFY_LITTLE_FAN, false);
        if (_winIndex != -1 && Common::isCStringEmpty(_detail.win_hand.tiles) && _detail.fan2_bits == 0 && _detail.fan1_bits == 0 && shouldShow) {
            showLittleFanAlert(true);
        }
        else {
            finish();
        }
    }
}

void RecordScene::finish() {
    _submitCallback(_detail);
    adjustRecentFans();
    Director::getInstance()->popScene();
}

// in FanCalculatorScene.cpp
extern cocos2d::ui::Widget *createFanResultWidget(const mahjong::fan_table_t &fan_table, int fontSize, float resultAreaWidth);

void RecordScene::showCalculator(const mahjong::calculate_param_t &param) {
    const float maxWidth = Director::getInstance()->getVisibleSize().width;

    // 选牌面板和其他信息的相关控件
    TilePickWidget *tilePicker = TilePickWidget::create(maxWidth - 10.0f);
    ExtraInfoWidget *extraInfo = ExtraInfoWidget::create(maxWidth - 10.0f, nullptr);

    extraInfo->setFlowerCount(param.flower_count);
    extraInfo->setPrevalentWind(static_cast<mahjong::wind_t>(_handIdx / 4));
    extraInfo->setSeatWind(param.seat_wind);

    const Size &pickerSize = tilePicker->getContentSize();
    const Size &extraInfoSize = extraInfo->getContentSize();

    // 布局在rootNode上
    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(maxWidth, pickerSize.height + extraInfoSize.height + 10.0f));
    rootNode->addChild(tilePicker);
    tilePicker->setPosition(Vec2(maxWidth * 0.5f, pickerSize.height * 0.5f + extraInfoSize.height + 10.0f));
    rootNode->addChild(extraInfo);
    extraInfo->setPosition(Vec2(maxWidth * 0.5f, extraInfoSize.height * 0.5f + 5.0f));

    tilePicker->setFixedPacksChangedCallback([tilePicker, extraInfo]() {
        extraInfo->refreshByKong(tilePicker->isFixedPacksContainsKong());
    });

    tilePicker->setWinTileChangedCallback([tilePicker, extraInfo]() {
        extraInfo->refreshByWinTile(tilePicker->getServingTile(), !tilePicker->isStandingTilesContainsServingTile(),
            tilePicker->countServingTileInFixedPacks(), tilePicker->isFixedPacksContainsKong());
    });

    if (param.hand_tiles.tile_count != 0 && param.win_tile != 0) {
        tilePicker->setData(param.hand_tiles, param.win_tile);
        extraInfo->setWinFlag(param.win_flag);
    }

    // 通过AlertDialog显示出来
    AlertDialog::Builder(this, maxWidth)
        .setTitle(__UTF8("记录和牌"))
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setNegativeButton(__UTF8("取消"), nullptr)
        .setPositiveButton(__UTF8("确定"), [this, tilePicker, extraInfo, param](AlertDialog *dlg, int) {
        mahjong::calculate_param_t temp = { 0 };
        tilePicker->getData(&temp.hand_tiles, &temp.win_tile);
        if (temp.win_tile == 0 && temp.hand_tiles.tile_count == 0 && temp.hand_tiles.pack_count == 0) {
            AlertDialog::Builder(this)
                .setTitle(__UTF8("记录和牌"))
                .setMessage(__UTF8("确定不记录和牌吗？"))
                .setPositiveButton(__UTF8("确定"), [this, dlg](AlertDialog *, int) { dlg->dismiss(); return true; })
                .setNegativeButton(__UTF8("取消"), nullptr)
                .create()->show();
            return false;
        }

        temp.flower_count = static_cast<uint8_t>(extraInfo->getFlowerCount());
        if (temp.flower_count > 8) {
            Toast::makeText(this, __UTF8("花牌数的范围为0~8"), Toast::LENGTH_LONG)->show();
            return false;
        }

        if (temp.win_tile == 0) {
            Toast::makeText(this, __UTF8("牌张数错误"), Toast::LENGTH_LONG)->show();
            return false;
        }

        std::sort(temp.hand_tiles.standing_tiles, temp.hand_tiles.standing_tiles + temp.hand_tiles.tile_count);

        mahjong::fan_table_t fan_table = { 0 };

        // 获取绝张、杠开、抢杠、海底信息
        temp.win_flag = extraInfo->getWinFlag();

        // 获取圈风门风
        temp.prevalent_wind = extraInfo->getPrevalentWind();
        temp.seat_wind = extraInfo->getSeatWind();

        // 算番
        int fan = mahjong::calculate_fan(&temp, &fan_table);

        if (fan == ERROR_NOT_WIN) {
            Toast::makeText(this, __UTF8("诈和"), Toast::LENGTH_LONG)->show();
            return false;
        }
        if (fan == ERROR_WRONG_TILES_COUNT) {
            Toast::makeText(this, __UTF8("牌张数错误"), Toast::LENGTH_LONG)->show();
            return false;
        }
        if (fan == ERROR_TILE_COUNT_GREATER_THAN_4) {
            Toast::makeText(this, __UTF8("同一种牌最多只能使用4枚"), Toast::LENGTH_LONG)->show();
            return false;
        }

        const float maxWidth = AlertDialog::maxWidth();

        ui::Widget *innerWidget = createFanResultWidget(fan_table, 12, maxWidth);
        const Size &fanResultSize = innerWidget->getContentSize();

        // 花（使用emoji代码）
        Label *flowerLabel = nullptr;
        if (temp.flower_count > 0) {
            flowerLabel = Label::createWithSystemFont(std::string(EMOJI_FLOWER_8, temp.flower_count * (sizeof(EMOJI_FLOWER) - 1)), "Arial", 12);
            flowerLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
            flowerLabel->setColor(Color3B(224, 45, 45));
#endif
        }

        // 手牌
        Node *tilesNode = HandTilesWidget::createStaticNode(temp.hand_tiles, temp.win_tile);
        Size tilesNodeSize = tilesNode->getContentSize();
        if (tilesNodeSize.width > maxWidth) {
            const float scale = maxWidth / tilesNodeSize.width;
            tilesNode->setScale(scale);
            tilesNodeSize.width = maxWidth;
            tilesNodeSize.height *= scale;
        }
        innerWidget->addChild(tilesNode);
        tilesNode->setPosition(Vec2(maxWidth * 0.5f, fanResultSize.height + 5 + tilesNodeSize.height * 0.5f));

        if (temp.flower_count > 0) {
            innerWidget->addChild(flowerLabel);
            const Size &flowerSize = flowerLabel->getContentSize();
            flowerLabel->setPosition(Vec2(0, fanResultSize.height + 5 + tilesNodeSize.height + 5 + flowerSize.height * 0.5f));

            innerWidget->setContentSize(Size(maxWidth, fanResultSize.height + 5 + tilesNodeSize.height + 5 + flowerSize.height));
        }
        else {
            innerWidget->setContentSize(Size(maxWidth, fanResultSize.height + 5 + tilesNodeSize.height));
        }

        uint64_t fanBits = 0;
        for (int n = mahjong::BIG_FOUR_WINDS; n < mahjong::DRAGON_PUNG; ++n) {
            if (fan_table[n]) {
                SET_FAN(fanBits, n);
            }
        }

        uint32_t fan2Bits = 0;
        uint64_t fan1Bits = 0;
        for (unsigned n = 0; n < 10; ++n) {
            auto cnt = fan_table[mahjong::DRAGON_PUNG + n];
            if (cnt > 0) {
                SET_FAN2(fan2Bits, n, cnt);
            }
        }
        for (unsigned n = 0;
#if SUPPORT_CONCEALED_KONG_AND_MELDED_KONG
            n < 14;
#else
            n < 13;
#endif
            ++n) {
            uint16_t cnt = fan_table[mahjong::PURE_DOUBLE_CHOW + n];
            if (cnt > 0) {
                SET_FAN1(fan1Bits, n, cnt);
            }
        }

        AlertDialog::Builder(this)
            .setTitle(__UTF8("记录和牌"))
            .setContentNode(innerWidget)
            .setCloseOnTouchOutside(false)
            .setPositiveButton(__UTF8("确定"), [this, temp, fan, fanBits, fan2Bits, fan1Bits, dlg](AlertDialog *, int) {
            _detail.fan = static_cast<uint16_t>(std::max(fan, 8));
            _detail.fan_bits = fanBits;
            _detail.fan2_bits = fan2Bits;
            _detail.fan1_bits = fan1Bits;

            Record::Detail::WinHand &winHand = _detail.win_hand;
            memset(&winHand, 0, sizeof(winHand));
            intptr_t n = mahjong::hand_tiles_to_string(&temp.hand_tiles, winHand.tiles, 64);
            mahjong::tiles_to_string(&temp.win_tile, 1, &winHand.tiles[n], sizeof(winHand.tiles) - n);

            winHand.win_flag = temp.win_flag;
            winHand.flower_count = temp.flower_count;

            // 更改输入框中的番数
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", fan);
            _editBox->setText(buf);

            // 根据记录和牌的门风确定和牌的是哪一家
            int winIndex = (static_cast<int>(temp.seat_wind) + (_handIdx & 0x3)) & 0x3;
            _winGroup->setSelectedButton(winIndex);

            if (temp.win_flag & WIN_FLAG_SELF_DRAWN) {  // 自摸
                _claimGroup->setSelectedButton(winIndex);
            }

            refresh();
            dlg->dismiss();
            return true;
        }).setNegativeButton(__UTF8("取消"), nullptr).create()->show();
        return false;
    }).create()->show();
}
