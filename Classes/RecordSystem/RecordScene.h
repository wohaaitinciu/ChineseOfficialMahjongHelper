#ifndef _RECORD_SCENE_H_
#define _RECORD_SCENE_H_

#include "../BaseScene.h"
#include "../cocos-wheels/CWTableView.h"
#include "Record.h"
#include "../mahjong-algorithm/fan_calculator.h"

class TilePickWidget;
class ExtraInfoWidget;

#define C4B_ORANGE      cocos2d::Color4B::ORANGE
#define C4B_RED         cocos2d::Color4B(254,  87, 110, 255)
#define C4B_BLUE        cocos2d::Color4B( 44, 121, 178, 255)
#define C4B_GREEN       cocos2d::Color4B( 45, 175,  90, 255)
#define C4B_PURPLE      cocos2d::Color4B(182, 118, 226, 255)

static const char *handNameText[] = {
    __UTF8("东风东"), __UTF8("东风南"), __UTF8("东风西"), __UTF8("东风北"),
    __UTF8("南风东"), __UTF8("南风南"), __UTF8("南风西"), __UTF8("南风北"),
    __UTF8("西风东"), __UTF8("西风南"), __UTF8("西风西"), __UTF8("西风北"),
    __UTF8("北风东"), __UTF8("北风南"), __UTF8("北风西"), __UTF8("北风北")
};

#define EMOJI_FLOWER "\xF0\x9F\x8C\xB8"

#define EMOJI_FLOWER_8 EMOJI_FLOWER EMOJI_FLOWER EMOJI_FLOWER EMOJI_FLOWER EMOJI_FLOWER EMOJI_FLOWER EMOJI_FLOWER EMOJI_FLOWER

class RecordScene : public BaseScene, cocos2d::ui::EditBoxDelegate, cw::TableViewDelegate {
public:
    typedef std::function<void (const Record::Detail &)> SubmitCallback;

    CREATE_FUNC_WITH_PARAM_5(RecordScene, unsigned, handIdx, const char **, names, uint8_t, mode, const Record::Detail *, detail, SubmitCallback &&, okCallback);
    bool init(unsigned handIdx, const char **names, uint8_t mode, const Record::Detail *detail, SubmitCallback &&callback);

    virtual void editBoxEditingDidBegin(cocos2d::ui::EditBox* editBox) override;
    virtual void editBoxReturn(cocos2d::ui::EditBox *editBox) override;

    static void SetScoreLabelColor(cocos2d::Label *(&scoreLabel)[4], int (&scoreTable)[4], uint8_t win_flag, uint8_t claim_flag, const int16_t (&penalty_scores)[4]);

private:
    cocos2d::ui::EditBox *_editBox = nullptr;
    cocos2d::ui::CheckBox *_drawBox = nullptr;
    cocos2d::ui::CheckBox *_timeoutBox = nullptr;
    cocos2d::ui::RadioButtonGroup *_winGroup = nullptr;
    cocos2d::ui::RadioButtonGroup *_claimGroup = nullptr;
    cocos2d::Label *_byDiscardLabel[4];
    cocos2d::Label *_selfDrawnLabel[4];
    cocos2d::Label *_scoreLabel[4];
    cocos2d::ui::RichText *_penaltyText[4];

    cocos2d::ui::Button *_recordTilesButton = nullptr;
    cocos2d::ui::Button *_littleFanButton = nullptr;
    cw::TableView *_tableView = nullptr;
    cocos2d::ui::Button *_submitButton = nullptr;

    unsigned _handIdx = 0;
    const char *_playerNames[4];
    uint8_t _mode = 0;
    int _winIndex = -1;
    Record::Detail _detail;
    SubmitCallback _submitCallback;
    uint8_t _seatFlag;
    uint8_t _playerFlag;

    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void refresh();
    void updateScoreLabel();
    void showLittleFanAlert(bool callFromSubmitting);

    void onInstructionButton(cocos2d::Ref *sender);
    void onPlusButton(cocos2d::Ref *sender);
    void onRecordTilesButton(cocos2d::Ref *sender);
    void onDrawTimeoutBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onPenaltyButton(cocos2d::Ref *sender);
    void onWinGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event);
    void onClaimGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event);

    void onFanNameBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onSubmitButton(cocos2d::Ref *sender);

    void adjustRecentFans();
    void finish();

    void showCalculator(const mahjong::calculate_param_t &param);
};

#endif
