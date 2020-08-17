#ifndef __SCORE_SHEET_SCENE_H__
#define __SCORE_SHEET_SCENE_H__

#include "../BaseScene.h"
#include "Record.h"

class ScoreSheetScene : public BaseScene {
public:
    virtual bool init() override;

    CREATE_FUNC(ScoreSheetScene);

private:
    float _cellWidth = 0.0f;
    cocos2d::Label *_titleLabel = nullptr;
    cocos2d::Label *_nameLabel[4];
    cocos2d::Label *_modeLabel = nullptr;
    cocos2d::ui::Button *_startButton = nullptr;
    cocos2d::ui::Button *_finishButton = nullptr;
    cocos2d::Label *_totalLabel[4];
    cocos2d::Label *_checkLabel = nullptr;
    cocos2d::Label *_scoreLabels[16][4];
    cocos2d::ui::Button *_recordButton[16];
    cocos2d::ui::Widget *_detailWidget[16];
    cocos2d::Label *_fanNameLabel[16];
    cocos2d::Label *_rankLabels[4];
    cocos2d::Label *_timeLabel = nullptr;

    Record _record;
    bool _isGlobal = false;
    bool _isTotalMode = false;

    bool initWithRecord(Record *record);

    void addUpScores(unsigned handIdx, int (&totalScores)[4]) const;
    void fillScoresForSingleMode(unsigned handIdx, int (&totalScores)[4]);
    void fillScoresForTotalMode(unsigned handIdx, int (&totalScores)[4]);
    void fillDetail(unsigned handIdx);
    void cleanRow(unsigned handIdx);
    void refreshRank(const int (&totalScores)[4]);
    void refreshTitle();
    void refreshEndTime();
    void refreshScores();
    void recover();
    void reset();
    void forceFinish();
    void onEditButton(cocos2d::Ref *sender);
    void editNameAndTitle();
    void onStartButton(cocos2d::Ref *sender);
    void onFinishButton(cocos2d::Ref *sender);
    void onRecordButton(cocos2d::Ref *sender);
    void onDetailButton(cocos2d::Ref *sender);
    void editRecord(unsigned handIdx, const Record::Detail *detail);
    void onTimeScheduler(float dt);
    void onMoreButton(cocos2d::Ref *sender);
    void showInstructionAlert();
    void showSettingAlert();
    void onHistoryButton(cocos2d::Ref *sender);
    void onResetButton(cocos2d::Ref *sender);
    void onPursuitButton(cocos2d::Ref *sender);
};

#endif // __SCORE_SHEET_SCENE_H__
