#ifndef __TRAINING_SCENE_H__
#define __TRAINING_SCENE_H__

#include "../BaseScene.h"

class HandTilesWidget;

class TrainingScene : public BaseScene {
public:
    virtual bool init() override;

    CREATE_FUNC(TrainingScene);

private:
    HandTilesWidget *_handTilesWidget = nullptr;
    cocos2d::ui::Widget *_shaderWidget = nullptr;
    cocos2d::Label *_countLabel = nullptr;
    cocos2d::Sprite *_answerTiles[2];
    cocos2d::ui::Button *_skipButton = nullptr;
    cocos2d::ui::Button *_answerButton = nullptr;
    cocos2d::ui::CheckBox *_jumpCheck = nullptr;
    std::vector<cocos2d::Node *> _resultNode;
    cocos2d::Node *_currentNode = nullptr;
    unsigned _countsPerRow = 0;
    cocos2d::Vec2 _resultPos;
    cocos2d::Rect _viewRect;
    cocos2d::Label *_modeLabel = nullptr;
    cocos2d::Label *_timeLabel = nullptr;
    cocos2d::LayerColor *_frontColor = nullptr;
    cocos2d::Label *_countdownLabel = nullptr;

    void loadPuzzle();
    void setPuzzle();
    void onStandingTileEvent();
    void onAnswerButton(cocos2d::Ref *sender);
    void onSkipButton(cocos2d::Ref *sender);
    void refreshRate();
    void addResult();
    void setResult(bool right);
    void removeAllResults();
    void onMoreButton(cocos2d::Ref *sender);
    void showNormalAlert();
    void showTimeSettingAlert();
    void startNormal();
    void startRush(uint8_t minutes);
    void doStartRush(uint8_t minutes);
    void finishRush(uint8_t minutes);
    void requestLatestPuzzles();

    uint64_t _answer = 0;
    uint32_t _totalCount = 0;
    uint32_t _rightCount = 0;
    bool _newPuzzle = false;
    bool _right = false;
    bool _autoJump = true;

    int64_t _finishTime = 0;
};

#endif
